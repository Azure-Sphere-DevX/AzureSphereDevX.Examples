Write-Output "`n`nBuild all test tool for AzureSphereDevX examples`n`n"

$gnupath = "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\Linux\gcc_arm\bin"
$exit_code = 0

function build-high-level {
    param (
        $dir
    )

    New-Item -Name "./build" -ItemType "directory" -ErrorAction SilentlyContinue
    Set-Location "./build"

    cmake `
        -G "Ninja" `
        -DCMAKE_TOOLCHAIN_FILE="C:\Program Files (x86)\Microsoft Azure Sphere SDK\CMakeFiles\AzureSphereToolchain.cmake" `
        -DCMAKE_BUILD_TYPE="Release" `
        $dir

    cmake --build .   
}

function build-real-time {
    param (
        $dir
    )

    New-Item -Name "./build" -ItemType "directory" -ErrorAction SilentlyContinue
    Set-Location "./build"

    cmake `
        -G "Ninja" `
        -DCMAKE_TOOLCHAIN_FILE="C:\Program Files (x86)\Microsoft Azure Sphere SDK\CMakeFiles\AzureSphereRTCoreToolchain.cmake" `
        -DARM_GNU_PATH:STRING=$gnupath `
        -DCMAKE_BUILD_TYPE="Release" `
        $dir

    cmake --build .
}

function build_application {

    param (
        $dir
    )

    $manifest = Join-Path -Path $dir -ChildPath "app_manifest.json"
    $cmakefile = Join-Path -Path $dir -ChildPath "CMakeLists.txt"    

    if (Test-Path -path $cmakefile) {

        # Only build executable apps not libraries
        if (Select-String -Path $cmakefile -Pattern 'add_executable' -Quiet ) {

            # Is this a high-level app?
            if (Select-String -Path $manifest -Pattern 'Default' -Quiet) {

                build-high-level $dir 
            }
            else {
                # Is this a real-time app?
                if (Select-String -Path $manifest -Pattern 'RealTimeCapable' -Quiet) {

                    build-real-time $dir
                }
                else {
                    # Not a high-level or real-time app so just continue
                    continue
                }
            } 

            # was the imagepackage created - this is a proxy for sucessful build
            $imagefile = Get-ChildItem "*.imagepackage" -Recurse
            if ($imagefile.count -ne 0) {
                Write-Output "`nSuccessful build: $dir.`n"
            }
            else {
                Set-Location ".."
                Write-Output "`nBuild failed: $dir.`n"

                $script:exit_code = 1
                
                break
            }

            Set-Location ".."
            Remove-Item -Recurse -Force -ErrorAction SilentlyContinue -path ./build
        }
    }
}

$StartTime = $(get-date)

Remove-Item -Recurse -Force -ErrorAction SilentlyContinue -path ./build

$files = Get-ChildItem -Recurse -Depth 3 -Filter app_manifest.json

# Write-Output "Building $files.count projects"
foreach ($file in $files) {
    Write-Output ( -join ("BUILDING:", $file.DirectoryName))
}

Write-Output "`n`nBuild all process starting.`n"


foreach ($file in $files) {
    Write-Output ( -join ("BUILDING:", $file.DirectoryName, "`n"))
    build_application $file.DirectoryName
}

Remove-Item -Recurse -Force -ErrorAction SilentlyContinue -path ./build

$elapsedTime = $(get-date) - $StartTime
$totalTime = "{0:HH:mm:ss}" -f ([datetime]$elapsedTime.Ticks)

if ($exit_code -eq 0) {
    Write-Output "Build All completed sucessfully. Elapsed time: $totalTime"
}
else {
    Write-Output "Build All failed. Elapsed time: $totalTime"
}

exit $exit_code