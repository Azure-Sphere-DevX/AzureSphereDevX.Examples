$tools_version = "?.?"

$azure_sphere_sdk_version = azsphere show-version -o tsv
$version_parts = $azure_sphere_sdk_version.split('.')
if ($version_parts.count -gt 2) {
    $local:tools_version=  $version_parts[0] + '.' + $version_parts[1]
}

Write-Output $tools_version