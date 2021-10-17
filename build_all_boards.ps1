$StartTime = $(get-date)

'set(AVNET TRUE "AVNET Azure Sphere Starter Kit Revision 1")' | Out-File -FilePath azsphere_board.global.txt
Write-Output "`n=================Building for Azure Sphere Developer Board========================="
Get-Content ./azsphere_board.global.txt
Write-Output "==================================================================================="
pwsh ./tools\build-tools\build_all.ps1

if ($?) {
    'set(AVNET_REV_2 TRUE "AVNET Azure Sphere Starter Kit Revision 2")' | Out-File -FilePath azsphere_board.global.txt
    Write-Output "`n=================Building for Azure Sphere Developer Board========================="
    Get-Content ./azsphere_board.global.txt
    Write-Output "==================================================================================="
    pwsh ./tools\build-tools\build_all.ps1
}

if ($?) {
    'set(SEEED_STUDIO_RDB TRUE "Seeed Studio Azure Sphere MT3620 Development Kit (aka Reference Design Board or RDB)")' | Out-File -FilePath azsphere_board.global.txt
    Write-Output "`n=================Building for Azure Sphere Developer Board========================="
    Get-Content ./azsphere_board.global.txt
    Write-Output "==================================================================================="
    pwsh ./tools\build-tools\build_all.ps1
}

if ($?) {
    'set(SEEED_STUDIO_MINI TRUE "Seeed Studio Azure Sphere MT3620 Mini Dev Board")' | Out-File -FilePath azsphere_board.global.txt
    Write-Output "`n=================Building for Azure Sphere Developer Board========================="
    Get-Content ./azsphere_board.global.txt
    Write-Output "==================================================================================="
    pwsh ./tools\build-tools\build_all.ps1
}

Remove-Item azsphere_board.global.txt

$elapsedTime = $(get-date) - $StartTime
$totalTime = "{0:HH:mm:ss}" -f ([datetime]$elapsedTime.Ticks)

if ($?) {
    Write-Output "Build All for All Board completed successfully. Elapsed time: $totalTime"
}
else {
    Write-Output "Build All for All Boards failed. Elapsed time: $totalTime"
}