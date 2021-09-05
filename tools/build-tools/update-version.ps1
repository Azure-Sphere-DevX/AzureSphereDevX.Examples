Write-Output "`n`nBuild all test tool for AzureSphereDevX examples`n`n"

$exit_code = 0
$target_api_version = "10"
$tools_version = "?.?"

function update_cmakelists {
    param (
        $dir, 
        $target_api_version,
        $tools_version
    )

    Write-Output $dir $target_api_version  $tools_version

    # $cmakefile = Join-Path -Path $dir -ChildPath "CMakeLists.txt" 

    # $data = Get-Content -path $cmakefile

    # # ((Get-Content -path $cmakefile -Raw) -replace 'TARGET_API_SET "11"','TARGET_API_SET "10"') | Set-Content -Path $cmakefile

    # foreach($line in $data){
    #     if ($line.Contains('TARGET_API_SET')){
    #         Write-Output $line
    #         $line = 'azsphere_configure_api(TARGET_API_SET "11")'
    #         Write-Output $line
    #     }
    # }

    # $data

    # $data | Set-Content -Path $cmakefile

}

function update_cmake_settings {
    param (
        $dir, 
        $target_api_version
    )

    Write-Output "`nBuilding: $dir`n"
    Write-Output "Target API Version: $target_api_version`n"

    $cmakefile = Join-Path -Path $dir -ChildPath "CMakeSettings.json" 

    $data = Get-Content -Path $cmakefile | ConvertFrom-JSON

    $vars = $data[0].configurations[0].variables

    foreach ($var in $vars) { 
        if ($var.name -eq "AZURE_SPHERE_TARGET_API_SET") { 
            $var.value = "$target_api_version" 
        }
    }

    $vars = $data[0].configurations[1].variables

    foreach ($var in $vars) { 
        if ($var.name -eq "AZURE_SPHERE_TARGET_API_SET") { 
            $var.value = "$target_api_version" 
        }
    }

    $data | ConvertTo-json -Depth 10  | Out-File  $cmakefile
}


function find_projects {

    param (
        $target_api_version,
        $tools_version
    )

    $dirlist = Get-ChildItem -Recurse -Directory -Name -Depth 2
    foreach ($dir in $dirList) {

        $manifest = Join-Path -Path $dir -ChildPath "app_manifest.json"
        $cmakefile = Join-Path -Path $dir -ChildPath "CMakeLists.txt"    

        if ((Test-Path -path $manifest) -and (Test-Path -path $cmakefile) ) {

            # Only build executable apps not libraries
            if (Select-String -Path $cmakefile -Pattern 'add_executable' -Quiet ) {

                # Is this a high-level app?
                if (Select-String -Path $manifest -Pattern 'Default' -Quiet) {

                    # update_cmake_settings $dir $target_api_version $tools_version
                    update_cmakelists $dir $target_api_version $tools_version
                }
                else {                   
                    continue
                } 
            }
        }
    }
}

Write-Output $PSScriptRoot

$tools_version = .$PSScriptRoot\get_tools_version.ps1
$target_api_version = .$PSScriptRoot\get_latest_sysroot.ps1

find_projects $target_api_version  $tools_version

exit $exit_code