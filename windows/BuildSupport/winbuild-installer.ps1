. ((Split-Path -Parent $MyInvocation.MyCommand.Path) + "\winbuild-utils.ps1")

# Split each key-value pair into hash table entires
$args | Foreach-Object {$argtable = @{}} {if ($_ -Match "(.*)=(.*)") {$argtable[$matches[1]] = $matches[2];}}
$dotNetURL = $argtable["dotNetURL"]

#Set up web client to download stuff
[System.Net.WebClient] $wclient = New-Object System.Net.WebClient

# Create directory to store bootstrapper packages
new-item -ItemType directory -Path ".\msi-installer\bootstrapper\packages" -Force

# Download dotNet installer
Write-Host ("Downloading dotNet from $dotNetURL")
$wclient.DownloadFile($dotNetURL, "./msi-installer/iso/windows/dotNetFx40_Full_x86_x64.exe")
if (!(Test-Path -Path "./msi-installer/iso/windows/dotNetFx40_Full_x86_x64.exe"))
{
	Write-Host ("Error: Failed to download: dotNetFx40_Full_x86_x64.exe")
	Write-Host ("Will try once more & once more only.")
        $wclient.DownloadFile($dotNetURL, "./msi-installer/iso/windows/dotNetFx40_Full_x86_x64.exe")
}

if (!(Test-Path -Path "./msi-installer/iso/windows/dotNetFx40_Full_x86_x64.exe"))
{
	Write-Host ("Error: Failed to download: dotNetFx40_Full_x86_x64.exe both times! Giving up.")
	ExitWithCode -exitcode $global:failure
}

# Move the newly compiled xensetup package to the packages directory
Copy-Item .\xc-windows\install\xensetup.exe .\msi-installer\bootstrapper\packages -Force -V

# Create directory to store the necessary merge modules
new-item -ItemType directory -Path ".\msi-installer\installer\Modules" -Force

#Copy in the win-tools bits
Copy-Item -Path .\win-tools\MSMs\bin\XenGuestPlugin.msm .\msi-installer\installer\Modules -Verbose
Copy-Item -Path .\win-tools\MSMs\bin\XenGuestPlugin64.msm .\msi-installer\installer\Modules -Verbose
Copy-Item -Path .\win-tools\MSMs\bin\XenGuestAgent.msm .\msi-installer\installer\Modules -Verbose
Copy-Item -Path .\win-tools\MSMs\bin\XenClientGuestService.msm .\msi-installer\installer\Modules -Verbose
Copy-Item -Path .\win-tools\MSMs\bin\Udbus.Bindings.Interfaces.Libraries.Installer.msm .\msi-installer\installer\Modules -Verbose
Copy-Item -Path .\win-tools\MSMs\bin\Udbus.Bindings.Client.Libraries.Installer.msm .\msi-installer\installer\Modules -Verbose
Copy-Item -Path .\win-tools\MSMs\bin\Udbus.Bindings.Service.Libraries.Installer.msm .\msi-installer\installer\Modules -Verbose

ExitWithCode -exitcode $global:success
