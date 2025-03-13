New-Variable os -Value 'Windows' -Option Constant

New-Variable version -Value ('18') -Option Constant
New-Variable latest -Value ($version -eq 'latest') -Option Constant
New-Variable x64 -Value ('x64' -eq 'x64') -Option Constant

# function Locate-Choco {
#     $path = Get-Command 'choco' -ErrorAction SilentlyContinue
#     if ($path) {
#         $path.Path
#     }
#     else {
#         Join-Path ${env:ProgramData} 'chocolatey' 'bin' 'choco'
#     }
# }

# function Install-Package {
#     param(
#         [Parameter(Mandatory = $true, ValueFromRemainingArguments = $true)]
#         [string[]] $Packages
#     )

#     $choco = Locate-Choco
#     & $choco upgrade $Packages -y --no-progress --allow-downgrade
# }

# $clang = 'clang'
# $clangxx = 'clang++'

# Install-Package llvm

# $bin_dir = Join-Path $env:ProgramFiles LLVM bin
# echo $bin_dir >> $env:GITHUB_PATH

# New-Variable cc -Value ('1' -eq '1') -Option Constant

# New-Variable clang -Value 'clang' -Option Constant
# New-Variable clangxx -Value 'clang++' -Option Constant

# function Link-Exe {
#     param(
#         [Parameter(Mandatory = $true)]
#         [string] $Exe,
#         [Parameter(Mandatory = $true)]
#         [string] $LinkName
#     )

#     $exe_path = (Get-Command $Exe).Path
#     $link_dir = Split-Path $exe_path
#     $link_name = "$LinkName.exe"
#     $link_path = Join-Path $link_dir $link_name
#     echo "Creating link $link_path -> $exe_path"
#     New-Item -ItemType HardLink -Path $link_path -Value $exe_path -Force | Out-Null
# }

# if ($cc) {
#     Link-Exe $clang cc
#     if ($clang -ne 'clang') {
#         Link-Exe $clang 'clang'
#     }
#     Link-Exe $clangxx c++
#     if ($clangxx -ne 'clang++') {
#         Link-Exe $clangxx 'clang++'
#     }
# }

# Load MSVC environment
$vcvarsPath = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"
$arch = "x64"  # Change to your desired architecture (e.g., x86, x64, arm, etc.)

cmd.exe /c " `"$vcvarsPath`" $arch && set " | ForEach-Object {
    if ($_ -match "^(.*?)=(.*)$") {
        Set-Item -Path "Env:\$($matches[1])" -Value "$($matches[2])"
    }
}

# Set environment variables
$ErrorActionPreference = "Stop"
$env:CC = "clang-cl.exe"
$env:CXX = "clang-cl.exe"
$env:VULKAN_SDK = "C:\VulkanSDK\"
$env:triplet = "x64-windows"
$env:vcpkg_baseline = "42bb0d9e8d4cf33485afb9ee2229150f79f61a1f"
$env:VCPKG_INSTALLED_DIR = "./vcpkg_installed/"
$env:dep_folder = "deps"
$env:libplacebo_tag = "v7.349.0"
$env:workplace = Split-Path -Parent $MyInvocation.MyCommand.Path
$env:python_path = "C:\Users\benir\anaconda3\envs\chiaki-ng"

# Set Qt environment variables
$env:QT_ROOT_DIR = "${env:workplace}\Qt\6.8.2\msvc2022_64"
$env:QT_PLUGIN_PATH = "${env:workplace}\Qt\6.8.2\msvc2022_64\plugins"
$env:QML2_IMPORT_PATH = "${env:workplace}\Qt\6.8.2\msvc2022_64\qml"
$env:Qt6_DIR = "${env:workplace}\Qt\6.8.2\msvc2022_64\lib\cmake\Qt6"
$env:QT_DIR = "${env:workplace}\Qt\6.8.2\msvc2022_64\lib\cmake\Qt6"

# Setup vcpkg
$env:RUNVCPKG_VCPKG_ROOT = "${env:workplace}\vcpkg"
$env:VCPKG_ROOT = "${env:workplace}\vcpkg"
$env:RUNVCPKG_VCPKG_ROOT_OUT = "${env:workplace}\vcpkg"
$env:VCPKG_DEFAULT_TRIPLET = "x64-windows"
$env:RUNVCPKG_VCPKG_DEFAULT_TRIPLET_OUT = "x64-windows"