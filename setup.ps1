New-Variable os -Value 'Windows' -Option Constant
  
New-Variable linux_host -Value ($os -eq 'Linux') -Option Constant
New-Variable cygwin_host -Value ('0' -eq '1') -Option Constant
New-Variable windows_host -Value ($os -eq 'Windows' -and !$cygwin_host) -Option Constant

New-Variable version -Value ('18') -Option Constant
New-Variable latest -Value ($version -eq 'latest') -Option Constant
New-Variable x64 -Value ('x64' -eq 'x64') -Option Constant

function Locate-Choco {
    $path = Get-Command 'choco' -ErrorAction SilentlyContinue
    if ($path) {
        $path.Path
    } else {
        Join-Path ${env:ProgramData} 'chocolatey' 'bin' 'choco'
    }
}

function Install-Package {
    param(
        [Parameter(Mandatory=$true, ValueFromRemainingArguments=$true)]
        [string[]] $Packages
    )

    if ($script:linux_host) {
        sudo apt-get update
        sudo DEBIAN_FRONTEND=noninteractive apt-get install -yq --no-install-recommends $Packages
    } elseif ($script:cygwin_host) {
        $choco = Locate-Choco
        & $choco install $Packages -y --no-progress --source=cygwin
    } elseif ($script:windows_host) {
        $choco = Locate-Choco
        & $choco upgrade $Packages -y --no-progress --allow-downgrade
    } else {
        throw "Sorry, installing packages is unsupported on $script:os"
    }
}

function Format-UpstreamVersion {
    param(
        [Parameter(Mandatory=$true)]
        [string] $Version
    )

    switch -Exact ($Version) {
        # Since version 7, they dropped the .0 suffix. The earliest
        # version supported is 3.9 on Bionic; versions 4, 5 and 6 are
        # mapped to LLVM-friendly 4.0, 5.0 and 6.0.
        '4' { '4.0' }
        '5' { '5.0' }
        '6' { '6.0' }
        default { $Version }
    }
}

function Format-AptLine {
    param(
        [Parameter(Mandatory=$true)]
        [string] $Version
    )

    if (!(Get-Command lsb_release -ErrorAction SilentlyContinue)) {
        throw "Couldn't find lsb_release; LLVM only provides repositories for Debian/Ubuntu"
    }
    $codename = lsb_release -sc

    "deb http://apt.llvm.org/$codename/ llvm-toolchain-$codename-$Version main"
}

function Add-UpstreamRepo {
    param(
        [Parameter(Mandatory=$true)]
        [string] $Version
    )

    wget -qO - https://apt.llvm.org/llvm-snapshot.gpg.key | sudo apt-key add -
    $apt_line = Format-AptLine $Version
    sudo add-apt-repository --yes --update $apt_line
}

$clang = 'clang'
$clangxx = 'clang++'

if ($linux_host) {
    $pkg_clang = 'clang'
    $pkg_llvm = 'llvm'
    $pkg_gxx = 'g++'

    if (!$latest) {
        $pkg_version = Format-UpstreamVersion $version
        Add-UpstreamRepo $pkg_version

        $pkg_clang = "$pkg_clang-$pkg_version"
        $pkg_llvm = "$pkg_llvm-$pkg_version"

        $clang = "$clang-$pkg_version"
        $clangxx = "$clangxx-$pkg_version"
    }
    if (!$x64) {
        $pkg_gxx = 'g++-multilib'
    }
    $packages = $pkg_clang,$pkg_llvm,$pkg_gxx

    Install-Package $packages
} elseif ($cygwin_host) {  
    # IDK why, but without libiconv-devel, even a "Hello, world!"
    # C++ app cannot be compiled as of December 2020. Also, libstdc++
    # is required; it's simpler to install gcc-g++ for all the
    # dependencies.
    Install-Package clang gcc-g++ libiconv-devel llvm
} elseif ($windows_host) {
    Install-Package llvm

    $bin_dir = Join-Path $env:ProgramFiles LLVM bin
    echo $bin_dir >> $env:GITHUB_PATH
} else {
    throw "Sorry, installing Clang is unsupported on $os"
}

New-Variable os -Value 'Windows' -Option Constant
  
New-Variable linux_host -Value ($os -eq 'Linux') -Option Constant
New-Variable cygwin_host -Value ('0' -eq '1') -Option Constant
New-Variable windows_host -Value ($os -eq 'Windows' -and !$cygwin_host) -Option Constant

New-Variable cc -Value ('1' -eq '1') -Option Constant

New-Variable clang -Value 'clang' -Option Constant
New-Variable clangxx -Value 'clang++' -Option Constant

function Link-Exe {
    param(
        [Parameter(Mandatory=$true)]
        [string] $Exe,
        [Parameter(Mandatory=$true)]
        [string] $LinkName
    )

    $exe_path = (Get-Command $Exe).Path
    $link_dir = if ($script:windows_host) { Split-Path $exe_path } else { '/usr/local/bin' }
    $link_name = if ($script:windows_host) { "$LinkName.exe" } else { $LinkName }
    $link_path = if ($script:cygwin_host) { "$link_dir/$link_name" } else { Join-Path $link_dir $link_name }
    echo "Creating link $link_path -> $exe_path"
    if ($script:linux_host) {
        sudo ln -f -s $exe_path $link_path
    } elseif ($script:cygwin_host) {
        ln.exe -f -s $exe_path $link_path
    } elseif ($script:windows_host) {
        New-Item -ItemType HardLink -Path $link_path -Value $exe_path -Force | Out-Null
    }
}

if ($cc) {
    Link-Exe $clang cc
    if ($clang -ne 'clang') {
        Link-Exe $clang 'clang'
    }
    Link-Exe $clangxx c++
    if ($clangxx -ne 'clang++') {
        Link-Exe $clangxx 'clang++'
    }
}


$env:CommandPromptType = "Native"
$env:DevEnvDir = "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\"
$env:ExtensionSdkDir = "C:\Program Files (x86)\Microsoft SDKs\Windows Kits\10\ExtensionSDKs"
$env:EXTERNAL_INCLUDE = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.35.32215\include;C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.35.32215\ATLMFC\include;C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\VS\include;C:\Program Files (x86)\Windows Kits\10\include\10.0.22000.0\ucrt;C:\Program Files (x86)\Windows Kits\10\\include\10.0.22000.0\\um;C:\Program Files (x86)\Windows Kits\10\\include\10.0.22000.0\\shared;C:\Program Files (x86)\Windows Kits\10\\include\10.0.22000.0\\winrt;C:\Program Files (x86)\Windows Kits\10\\include\10.0.22000.0\\cppwinrt"
$env:Framework40Version = "v4.0"
$env:FrameworkDir = "C:\Windows\Microsoft.NET\Framework64\"
$env:FrameworkDir64 = "C:\Windows\Microsoft.NET\Framework64\"
$env:FrameworkVersion = "v4.0.30319"
$env:FrameworkVersion64 = "v4.0.30319"
$env:FSHARPINSTALLDIR = "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\FSharp\Tools"
$env:HTMLHelpDir = "$env:HTMLHelpDir"
$env:IFCPATH = "$env:IFCPATH"
$env:INCLUDE = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.35.32215\include;C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.35.32215\ATLMFC\include;C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\VS\include;C:\Program Files (x86)\Windows Kits\10\include\10.0.22000.0\ucrt;C:\Program Files (x86)\Windows Kits\10\\include\10.0.22000.0\\um;C:\Program Files (x86)\Windows Kits\10\\include\10.0.22000.0\\shared;C:\Program Files (x86)\Windows Kits\10\\include\10.0.22000.0\\winrt;C:\Program Files (x86)\Windows Kits\10\\include\10.0.22000.0\\cppwinrt"
$env:is_x64_arch = "true"
$env:LIB = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.35.32215\ATLMFC\lib\x64;C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.35.32215\lib\x64;C:\Program Files (x86)\Windows Kits\10\lib\10.0.22000.0\ucrt\x64;C:\Program Files (x86)\Windows Kits\10\\lib\10.0.22000.0\\um\x64;C:\VulkanSDK\1.4.304.1\Lib;$env:LIB"
$env:LIBPATH = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.35.32215\ATLMFC\lib\x64;C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.35.32215\lib\x64;C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.35.32215\lib\x86\store\references;C:\Program Files (x86)\Windows Kits\10\UnionMetadata\10.0.22000.0;C:\Program Files (x86)\Windows Kits\10\References\10.0.22000.0;C:\Windows\Microsoft.NET\Framework64\v4.0.30319"
$env:NETFXSDKDir = "$env:NETFXSDKDir"
$env:Path = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.35.32215\bin\HostX64\x64;C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\VC\VCPackages;C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\TestWindow;C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\TeamFoundation\Team Explorer;C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\bin\Roslyn;C:\Program Files\Microsoft Visual Studio\2022\Community\Team Tools\Performance Tools\x64;C:\Program Files\Microsoft Visual Studio\2022\Community\Team Tools\Performance Tools;C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\FSharp\Tools;C:\Program Files (x86)\Windows Kits\10\bin\10.0.22000.0\\x64;C:\Program Files (x86)\Windows Kits\10\bin\\x64;C:\Program Files\Microsoft Visual Studio\2022\Community\\MSBuild\Current\Bin\amd64;C:\Windows\Microsoft.NET\Framework64\v4.0.30319;C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\;C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\;C:\Users\benir\anaconda3\condabin;C:\VulkanSDK\Bin;C:\Program Files\ImageMagick-7.1.1-Q16-HDRI;C:\Program Files\Common Files\Oracle\Java\javapath;C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v11.8\bin;C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v11.8\libnvvp;C:\Program Files (x86)\Razer Chroma SDK\bin;C:\Program Files\Razer Chroma SDK\bin;C:\Program Files (x86)\Razer\ChromaBroadcast\bin;C:\Program Files\Razer\ChromaBroadcast\bin;C:\Windows\system32;C:\Windows;C:\Windows\System32\Wbem;C:\Windows\System32\WindowsPowerShell\v1.0\;C:\Windows\System32\OpenSSH\;C:\Program Files\Git\cmd;C:\Program Files (x86)\NVIDIA Corporation\PhysX\Common;C:\Program Files\PuTTY\;C:\Users\benir\AppData\Roaming\nvm;C:\Program Files\nodejs;C:\Program Files\dotnet\;C:\Windows\system32\config\systemprofile\AppData\Local\Microsoft\WindowsApps;C:\Users\marco\AppData\Local\Microsoft\WindowsApps;C:\Program Files\NVIDIA Corporation\Nsight Compute 2022.3.0\;C:\Program Files\gs\gs10.03.1\bin;C:\ProgramData\chocolatey\bin;C:\WINDOWS\system32;C:\WINDOWS;C:\WINDOWS\System32\Wbem;C:\WINDOWS\System32\WindowsPowerShell\v1.0\;C:\WINDOWS\System32\OpenSSH\;C:\Program Files\NVIDIA Corporation\NVIDIA app\NvDLISR;C:\Program Files\LLVM\bin;C:\Program Files (x86)\LLVM\bin;C:\Users\benir\AppData\Local\Programs\Python\Python311\Scripts\;C:\Users\benir\AppData\Local\Programs\Python\Python311\;C:\Users\benir\AppData\Local\Microsoft\WindowsApps;C:\Users\benir\AppData\Local\Programs\Microsoft VS Code\bin;C:\texlive\2022\bin\win32;C:\Users\benir\AppData\Roaming\nvm;C:\Program Files\nodejs;C:\Users\benir\Documents\Projects\chiaki-ng\vcpkg;;C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\Llvm\x64\bin;C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin;C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja;C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\VC\Linux\bin\ConnectionManagerExe"
$env:Platform = "x64"
$env:UCRTVersion = "10.0.22000.0"
$env:UniversalCRTSdkDir = "C:\Program Files (x86)\Windows Kits\10\"
$env:VCIDEInstallDir = "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\VC\"
$env:VCINSTALLDIR = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\"
$env:VCPKG_ROOT = "$env:VCPKG_ROOT"
$env:VCToolsInstallDir = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.35.32215\"
$env:VCToolsRedistDir = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC\14.34.31931\"
$env:VCToolsVersion = "14.35.32215"
$env:VisualStudioVersion = "17.0"
$env:VS170COMNTOOLS = "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\"
$env:VSCMD_ARG_app_plat = "Desktop"
$env:VSCMD_ARG_HOST_ARCH = "x64"
$env:VSCMD_ARG_TGT_ARCH = "x64"
$env:VSCMD_VER = "17.5.4"
$env:VSINSTALLDIR = "C:\Program Files\Microsoft Visual Studio\2022\Community\"
$env:VSSDK150INSTALL = "$env:VSSDK150INSTALL"
$env:VSSDKINSTALL = "$env:VSSDKINSTALL"
$env:WindowsLibPath = "C:\Program Files (x86)\Windows Kits\10\UnionMetadata\10.0.22000.0;C:\Program Files (x86)\Windows Kits\10\References\10.0.22000.0"
$env:WindowsSdkBinPath = "C:\Program Files (x86)\Windows Kits\10\bin\"
$env:WindowsSdkDir = "C:\Program Files (x86)\Windows Kits\10\"
$env:WindowsSDKLibVersion = "10.0.22000.0\"
$env:WindowsSdkVerBinPath = "C:\Program Files (x86)\Windows Kits\10\bin\10.0.22000.0\"
$env:WindowsSDKVersion = "10.0.22000.0\"
$env:WindowsSDK_ExecutablePath_x64 = "$env:WindowsSDK_ExecutablePath_x64"
$env:WindowsSDK_ExecutablePath_x86 = "$env:WindowsSDK_ExecutablePath_x86"
$env:__DOTNET_ADD_64BIT = "1"
$env:__DOTNET_PREFERRED_BITNESS = "64"
$env:__VSCMD_PREINIT_PATH = "C:\Users\benir\anaconda3\condabin;C:\VulkanSDK\Bin;C:\Program Files\ImageMagick-7.1.1-Q16-HDRI;C:\Program Files\Common Files\Oracle\Java\javapath;C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v11.8\bin;C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v11.8\libnvvp;C:\Program Files (x86)\Razer Chroma SDK\bin;C:\Program Files\Razer Chroma SDK\bin;C:\Program Files (x86)\Razer\ChromaBroadcast\bin;C:\Program Files\Razer\ChromaBroadcast\bin;C:\Windows\system32;C:\Windows;C:\Windows\System32\Wbem;C:\Windows\System32\WindowsPowerShell\v1.0\;C:\Windows\System32\OpenSSH\;C:\Program Files\Git\cmd;C:\Program Files (x86)\NVIDIA Corporation\PhysX\Common;C:\Program Files\PuTTY\;C:\Users\benir\AppData\Roaming\nvm;C:\Program Files\nodejs;C:\Program Files\dotnet\;C:\Windows\system32\config\systemprofile\AppData\Local\Microsoft\WindowsApps;C:\Users\marco\AppData\Local\Microsoft\WindowsApps;C:\Program Files\NVIDIA Corporation\Nsight Compute 2022.3.0\;C:\Program Files\gs\gs10.03.1\bin;C:\ProgramData\chocolatey\bin;C:\WINDOWS\system32;C:\WINDOWS;C:\WINDOWS\System32\Wbem;C:\WINDOWS\System32\WindowsPowerShell\v1.0\;C:\WINDOWS\System32\OpenSSH\;C:\Program Files\NVIDIA Corporation\NVIDIA app\NvDLISR;C:\Program Files\LLVM\bin;C:\Program Files (x86)\LLVM\bin;C:\Users\benir\AppData\Local\Programs\Python\Python311\Scripts\;C:\Users\benir\AppData\Local\Programs\Python\Python311\;C:\Users\benir\AppData\Local\Microsoft\WindowsApps;C:\Users\benir\AppData\Local\Programs\Microsoft VS Code\bin;C:\texlive\2022\bin\win32;C:\Users\benir\AppData\Roaming\nvm;C:\Program Files\nodejs;C:\Users\benir\Documents\Projects\chiaki-ng\vcpkg;"

$ErrorActionPreference = "Stop"
$env:CC = "clang-cl.exe"
$env:CXX = "clang-cl.exe"
$env:VULKAN_SDK = "C:\VulkanSDK\"
$env:triplet = "x64-windows"
$env:vcpkg_baseline = "42bb0d9e8d4cf33485afb9ee2229150f79f61a1f"
$env:VCPKG_INSTALLED_DIR = "./vcpkg_installed/"
$env:dep_folder = "deps"
$env:libplacebo_tag = "v7.349.0"
$env:workplace = "C:\Users\benir\Documents\Projects\chiaki-ng"
$env:python_path = "C:\Users\benir\anaconda3\envs\chiaki-ng"

# Install Vulkan SDK
$ver = (Invoke-WebRequest -Uri "https://vulkan.lunarg.com/sdk/latest.json" | ConvertFrom-Json).windows
Write-Host "Installing Vulkan SDK Version ${ver}"
# Invoke-WebRequest -Uri "https://sdk.lunarg.com/sdk/download/1.4.304.1/windows/VulkanSDK-1.4.304.1-Installer.exe" -OutFile "VulkanSDK.exe"
Start-Process -Verb RunAs -Wait -FilePath "VulkanSDK.exe" -ArgumentList "--root ${env:VULKAN_SDK} --accept-licenses --default-answer --confirm-command install"
Remove-Item "VulkanSDK.exe"

# Install Python and dependencies
python -m pip install --upgrade pip setuptools wheel scons protobuf grpcio-tools pyinstaller meson

# Invoke-WebRequest -Uri "https://github.com/r52/FFmpeg-Builds/releases/download/latest/ffmpeg-n7.1-latest-win64-gpl-shared-7.1.zip" -OutFile ".\ffmpeg-n7.1-latest-win64-gpl-shared-7.1.zip"
Expand-Archive -LiteralPath "ffmpeg-n7.1-latest-win64-gpl-shared-7.1.zip" -DestinationPath "."
Rename-Item "ffmpeg-n7.1-latest-win64-gpl-shared-7.1" "${env:dep_folder}"
Remove-Item "ffmpeg-n7.1-latest-win64-gpl-shared-7.1.zip"

# Install QT
python.exe -m pip install setuptools wheel py7zr==0.20.*
python.exe -m pip install aqtinstall==3.1.*
python.exe -m aqt version
python.exe -m aqt install-qt windows desktop 6.8.* win64_msvc2022_64 --autodesktop --outputdir ${env:workplace}\Qt --modules qtwebengine qtpositioning qtwebchannel qtwebsockets qtserialport

$env:QT_ROOT_DIR = "${env:workplace}\Qt\6.8.2\msvc2022_64"
$env:QT_PLUGIN_PATH = "${env:workplace}\Qt\6.8.2\msvc2022_64\plugins"
$env:QML2_IMPORT_PATH = "${env:workplace}\Qt\6.8.2\msvc2022_64\qml"
$env:Qt6_DIR = "${env:workplace}\Qt\6.8.2\msvc2022_64\lib\cmake\Qt6"
$env:QT_DIR = "${env:workplace}\Qt\6.8.2\msvc2022_64\lib\cmake\Qt6"

# Build SPIRV-Cross
if (!(Test-Path "SPIRV-Cross")) {
    git clone https://github.com/KhronosGroup/SPIRV-Cross.git
}
cd SPIRV-Cross
cmake `
    -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_INSTALL_PREFIX="${env:workplace}\${env:dep_folder}" `
    -DSPIRV_CROSS_SHARED=ON `
    -S . `
    -B build `
    -G Ninja
cmake --build build --config Release
cmake --install build
cd ..

# Setup shaderc
$url = ((Invoke-WebRequest -UseBasicParsing -Uri "https://storage.googleapis.com/shaderc/badges/build_link_windows_vs2019_release.html").Content | Select-String -Pattern 'url=(.*)"').Matches.Groups[1].Value
# Invoke-WebRequest -UseBasicParsing -Uri ${url} -OutFile .\shaderc.zip
Expand-Archive -LiteralPath "shaderc.zip" -DestinationPath "."
cp "./install/*" "./${env:dep_folder}" -Force -Recurse
rm "./install" -r -force
Remove-Item "shaderc.zip"


# Setup vcpkg
if (!(Test-Path "vcpkg")) {
    git clone https://github.com/microsoft/vcpkg.git
    cd vcpkg
    git checkout ${env:vcpkg_baseline}
    .\bootstrap-vcpkg.bat
    cd ..
}
vcpkg install --recurse --clean-after-build --x-install-root ./vcpkg_installed/ --triplet=${env:triplet}

$env:RUNVCPKG_VCPKG_ROOT = "${env:workplace}\vcpkg"
$env:VCPKG_ROOT = "${env:workplace}\vcpkg"
$env:RUNVCPKG_VCPKG_ROOT_OUT = "${env:workplace}\vcpkg"
$env:VCPKG_DEFAULT_TRIPLET = "x64-windows"
$env:RUNVCPKG_VCPKG_DEFAULT_TRIPLET_OUT = "x64-windows"

# Build libplacebo
if (!(Test-Path "libplacebo")) {
    git clone --recursive https://github.com/haasn/libplacebo.git
}
cd libplacebo
git checkout --recurse-submodules v7.349.0
dos2unix ../scripts/flatpak/0002-Vulkan-use-16bit-for-p010.patch
git apply --ignore-whitespace --verbose ../scripts/flatpak/0002-Vulkan-use-16bit-for-p010.patch
meson setup `
    --prefix "${env:workplace}\${env:dep_folder}" `
    --native-file ../meson.ini `
    "--pkg-config-path=['${env:workplace}\vcpkg_installed\x64-windows\lib\pkgconfig','${env:workplace}\vcpkg_installed\x64-windows\share\pkgconfig','${env:workplace}\${env:dep_folder}\lib\pkgconfig']" `
    "--cmake-prefix-path=['${env:workplace}\vcpkg_installed\x64-windows', '${env:VULKAN_SDK}', '${env:workplace}\${env:dep_folder}']" `
    -Dc_args="/I ${env:VULKAN_SDK}Include" `
    -Dcpp_args="/I ${env:VULKAN_SDK}Include" `
    -Dc_link_args="/LIBPATH:${env:VULKAN_SDK}Lib" `
    -Dcpp_link_args="/LIBPATH:${env:VULKAN_SDK}Lib" `
    -Ddemos=false `
    ./build
ninja -C./build
ninja -C./build install
cd ..

# Apply Patches
git submodule update --init --recursive
git apply --ignore-whitespace --verbose --directory=third-party/gf-complete/ scripts/windows-vc/gf-complete.patch
git apply --ignore-whitespace --verbose scripts/windows-vc/libplacebo-pc.patch

# Configure chiaki-ng
cmake `
    -S . `
    -B build-debug `
    -G Ninja `
    -DCMAKE_TOOLCHAIN_FILE:STRING="vcpkg/scripts/buildsystems/vcpkg.cmake" `
    -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE `
    -DCMAKE_BUILD_TYPE=Debug `
    -DCHIAKI_ENABLE_CLI=OFF `
    -DCHIAKI_GUI_ENABLE_SDL_GAMECONTROLLER=ON `
    -DCHIAKI_ENABLE_STEAMDECK_NATIVE=OFF `
    -DPYTHON_EXECUTABLE="${env:python_path}\python.exe" `
    -DCMAKE_PREFIX_PATH="${env:workplace}\${env:dep_folder}; ${env:VULKAN_SDK}"

# Build chiaki-ng
cmake --build build-debug --config Debug --clean-first --target chiaki

# Prepare Qt deployment package
mkdir chiaki-ng-Win-debug
cp build-debug\gui\chiaki.exe chiaki-ng-Win-debug
cp build-debug\third-party\cpp-steam-tools\cpp-steam-tools.dll chiaki-ng-Win-debug
cp scripts\qtwebengine_import.qml gui\src\qml\
cp "${env:workplace}\vcpkg_installed\x64-windows\bin\libcrypto-*-x64.dll" chiaki-ng-Win-debug
cp "${env:workplace}\vcpkg_installed\x64-windows\bin\libssl-*-x64.dll" chiaki-ng-Win-debug
cp "${env:workplace}\vcpkg_installed\x64-windows\bin\SDL2.dll" chiaki-ng-Win-debug
cp "${env:workplace}\vcpkg_installed\x64-windows\bin\hidapi.dll" chiaki-ng-Win-debug
cp "${env:workplace}\vcpkg_installed\x64-windows\bin\fftw3.dll" chiaki-ng-Win-debug
cp "${env:workplace}\vcpkg_installed\x64-windows\bin\opus.dll" chiaki-ng-Win-debug
cp "${env:workplace}\vcpkg_installed\x64-windows\bin\libspeexdsp.dll" chiaki-ng-Win-debug
cp "${env:workplace}\vcpkg_installed\x64-windows\bin\lcms2.dll" chiaki-ng-Win-debug
cp "${env:workplace}\vcpkg_installed\x64-windows\bin\miniupnpc.dll" chiaki-ng-Win-debug
cp "${env:workplace}\vcpkg_installed\x64-windows\bin\json-c.dll" chiaki-ng-Win-debug
cp "${env:workplace}\vcpkg_installed\x64-windows\bin\zlib1.dll" chiaki-ng-Win-debug/zlib.dll
cp "${env:workplace}\${env:dep_folder}\bin\swresample-*.dll" chiaki-ng-Win-debug
cp "${env:workplace}\${env:dep_folder}\bin\avcodec-*.dll" chiaki-ng-Win-debug
cp "${env:workplace}\${env:dep_folder}\bin\avutil-*.dll" chiaki-ng-Win-debug
cp "${env:workplace}\${env:dep_folder}\bin\avformat-*.dll" chiaki-ng-Win-debug
cp "${env:workplace}\${env:dep_folder}\bin\libplacebo-*.dll" chiaki-ng-Win-debug
cp "${env:workplace}\${env:dep_folder}\bin\shaderc_shared.dll" chiaki-ng-Win-debug
cp "${env:workplace}\${env:dep_folder}\bin\spirv-cross-c-shared.dll" chiaki-ng-Win-debug
.\Qt\6.8.2\msvc2022_64\bin\windeployqt.exe --no-translations --qmldir=gui\src\qml --release chiaki-ng-Win-debug\chiaki.exe