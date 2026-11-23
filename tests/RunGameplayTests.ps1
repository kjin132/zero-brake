$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$installation = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -property installationPath
if (-not $installation) { throw 'Visual Studio C++ build tools are required.' }
$msbuild = Join-Path $installation 'MSBuild\Current\Bin\MSBuild.exe'
$build = Join-Path $root 'build\tests'
[IO.Directory]::CreateDirectory($build) | Out-Null
$xml = [xml](Get-Content -LiteralPath (Join-Path $root 'ZeroBrake\ZeroBrake.vcxproj') -Raw)
$ns = New-Object Xml.XmlNamespaceManager($xml.NameTable)
$ns.AddNamespace('m', $xml.DocumentElement.NamespaceURI)
foreach ($node in $xml.SelectNodes('//m:ClCompile[@Include]|//m:ClInclude[@Include]|//m:None[@Include]', $ns)) {
    $node.SetAttribute('Include', (Join-Path "$root\ZeroBrake" $node.GetAttribute('Include')))
    if ($node.Name -eq 'ClCompile') { $node.SetAttribute('Exclude', "$root\ZeroBrake\src\main.cpp") }
}
foreach ($node in $xml.SelectNodes('//m:AdditionalIncludeDirectories', $ns)) { $node.InnerText = "$root\ZeroBrake\src;%(AdditionalIncludeDirectories)" }
foreach ($compile in $xml.SelectNodes('//m:ItemDefinitionGroup/m:ClCompile', $ns)) {
    foreach ($setting in @{ MultiProcessorCompilation='true'; Optimization='MaxSpeed'; BasicRuntimeChecks='Default' }.GetEnumerator()) {
        $node = $xml.CreateElement($setting.Key, $xml.DocumentElement.NamespaceURI)
        $node.InnerText = $setting.Value
        [void]$compile.AppendChild($node)
    }
}
foreach ($node in $xml.SelectNodes('//m:PostBuildEvent/m:Command', $ns)) { $node.InnerText = 'echo Gameplay tests built' }
foreach ($node in $xml.SelectNodes('//m:OutDir', $ns)) { $node.InnerText = "$build\" }
foreach ($node in $xml.SelectNodes('//m:IntDir', $ns)) { $node.InnerText = "$build\obj\" }
$group = $xml.CreateElement('ItemGroup', $xml.DocumentElement.NamespaceURI)
$entry = $xml.CreateElement('ClCompile', $xml.DocumentElement.NamespaceURI)
$entry.SetAttribute('Include', (Join-Path $PSScriptRoot 'GameplayTests.cpp'))
[void]$group.AppendChild($entry)
[void]$xml.DocumentElement.AppendChild($group)
$project = Join-Path $build 'GameplayTests.vcxproj'
$xml.Save($project)
& $msbuild $project /p:Configuration=Debug /p:Platform=x64 /m /v:minimal /nologo
if ($LASTEXITCODE) { throw 'Test build failed' }
# Isolate persistent-record tests from the player's real records.
$run = Join-Path $build 'run'
[IO.Directory]::CreateDirectory($run) | Out-Null
Push-Location $run
try {
    & "$build\GameplayTests.exe"
    if ($LASTEXITCODE) { throw 'Gameplay tests failed' }
    & "$build\GameplayTests.exe" --read-record
    if ($LASTEXITCODE) { throw 'Ghost reload test failed' }
} finally { Pop-Location }
