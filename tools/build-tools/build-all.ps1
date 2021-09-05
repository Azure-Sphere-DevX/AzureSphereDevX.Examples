Write-Output "`n`nBuild all test tool for AzureSphereDevX examples`n`n"

$gnupath = "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\Linux\gcc_arm\bin"

Remove-Item -Recurse -Force -ErrorAction SilentlyContinue -path ./build

$dirlist = Get-ChildItem -Recurse -Directory -Name -Depth 2
foreach ($dir in $dirList) {

    $manifest = Join-Path -Path $dir -ChildPath "app_manifest.json"
    $cmakefile = Join-Path -Path $dir -ChildPath "CMakeLists.txt"    

    if ((Test-Path -path $manifest) -and (Test-Path -path $cmakefile) ) {

        # Only build executable apps not libraries
        if (Select-String -Path $cmakefile -Pattern 'add_executable' -Quiet ) {

            # Is this a high-level app?
            if (Select-String -Path $manifest -Pattern 'Default' -Quiet) {

                Write-Output "`nBuilding: $dir`n"

                New-Item -Name "./build" -ItemType "directory" -ErrorAction SilentlyContinue
                Set-Location "./build"

                cmake `
                    -G "Ninja" `
                    -DCMAKE_TOOLCHAIN_FILE="C:\Program Files (x86)\Microsoft Azure Sphere SDK\CMakeFiles\AzureSphereToolchain.cmake" `
                    -DCMAKE_BUILD_TYPE="Release" `
                    ../$dir

                cmake --build .

            }
            else {

                # Is this a real-time app?
                if (Select-String -Path $manifest -Pattern 'RealTimeCapable' -Quiet) {
                    Write-Output "`nBuilding Real-time: $dir`n"

                    New-Item -Name "./build" -ItemType "directory" -ErrorAction SilentlyContinue
                    Set-Location "./build"

                    cmake `
                        -G "Ninja" `
                        -DCMAKE_TOOLCHAIN_FILE="C:\Program Files (x86)\Microsoft Azure Sphere SDK\CMakeFiles\AzureSphereRTCoreToolchain.cmake" `
                        -DARM_GNU_PATH:STRING=$gnupath `
                        -DCMAKE_BUILD_TYPE="Release" `
                        ../$dir

                    cmake --build .
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
                break
            }

            Set-Location ".."
            Remove-Item -Recurse -Force -ErrorAction SilentlyContinue -path ./build
        }
    }
}

Remove-Item -Recurse -Force -ErrorAction SilentlyContinue -path ./build