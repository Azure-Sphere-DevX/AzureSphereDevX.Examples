# Introduction

The AzureSphereDevX library and the AzureSphere.Examples are community maintained repositories.

## Clone the Azure Sphere DevX examples

1. Clone the examples.

    ```bash
    git clone --recurse-submodules https://github.com/Avnet/AzureSphereDevX.Examples.git
    ```

1. Run the Update and Build tools. Follow the [build tools](https://github.com/Azure-Sphere-DevX/AzureSphereDevX.Examples/wiki/Build-Tools) notes to update and test build all examples.

## Azure Sphere DevX Documentation

Visit the [DevX library Wiki](https://github.com/Azure-Sphere-DevX/AzureSphereDevX.Examples/wiki) page to learn more.

## Azure Sphere DevX Overview

The DevX library accelerates your development and will help to improve your developer experience building  Azure Sphere applications.

The DevX library addresses many common Azure Sphere scenarios with a focus on:

1. IoT Hub messaging
1. IoT Hub Device Twins and Direct Methods
1. Intercore Messaging
1. Event Timers
1. GPIO.

The DevX library will help reduce the amount of code you write and improve readability and long-term application maintenance.

The focus of the Azure Sphere DevX library is the communications and simplification of common scenarios when building Azure Sphere applications.

## Azure Sphere GenX

Be sure to visit the [Azure Sphere GenX Code Generator WiKi](https://github.com/Azure-Sphere-DevX/AzureSphereGenX/wiki) partner project.

## Contributing to the DevX Examples Repo

This is an community maintained project and your contribution is welcome and appreciated. Please find the instructions below for adding additional examples to this repo.

1. Fork the repo into your GitHub account
2. Pull your repo, make sure to pull the submodules: ```git clone --recurse-submodules https://github.com/<your gitHub account name>/AzureSphereDevX.Examples.git```
3. From the command line, change to the root directory of the repo you pulled
4. Create a new branch for your example: ```git branch <yourBranchName>```
5. Switch to your new branch: ``` git checkout <yourBranchName>```
6. Add a new folder for your example, try to follow the existing example naming convention 
7. I usually copy an existing example as a starting point
8. Your new folder should NOT contain the AzureSphereDevX or HardwareDefinition folders, we'll add these as git submodules next
9. Add the submodules
10. Add the AzureSphereDevX submodule with the command: ```git submodule add https://github.com/Azure-Sphere-DevX/AzureSphereDevX.git .\<yourNewDirectory>\AzureSphereDevX```
12. Add the HardwareDefinitions submodule with the command: ```git submodule add https://github.com/Azure-Sphere-DevX/AzureSphereDevX.HardwareDefinitions.git .\<yourNewDirectory>\HardwareDefinitions``` 
13. Develop your application and check in your changes to your branch
14. Check the build using the /tools/build-tools/build_all script
15. Once you're ready to move your changes into the public repo open a Pull Request
