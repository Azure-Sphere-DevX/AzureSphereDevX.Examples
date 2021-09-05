
$sysroots = get-childitem "C:\Program Files (x86)\Microsoft Azure Sphere SDK\Sysroots"  | Sort-Object -Descending -Property {$_.Name -as [int]}

Write-Output  $sysroots[0].Name