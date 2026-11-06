$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
Push-Location $projectRoot
try {
    $compiler = Join-Path $projectRoot '.tools\ziglang\zig.exe'
    $glfwRoot = Join-Path $projectRoot '.tools\glfw-3.4.bin.WIN64'
    if (!(Test-Path -LiteralPath $compiler) -or !(Test-Path -LiteralPath $glfwRoot)) {
        throw 'Ferramentas locais ausentes. Use as instrucoes CMake do README.md.'
    }
    New-Item -ItemType Directory -Force -Path build | Out-Null
    $sources = @(
        'src\main.c', 'src\museum.c', 'src\camera.c', 'src\building.c',
        'src\interaction.c', 'src\geometry.c', 'src\artwork_render.c',
        'src\scene.c', 'src\textures.c', 'src\self_test.c', 'src\lighting.c',
        'src\balance.c', 'src\exhibits.c'
    )
    & $compiler cc -std=c99 -Wall -Wextra -Wpedantic -O2 @sources `
        "-I$projectRoot\include" "-I$glfwRoot\include" "-L$glfwRoot\lib-mingw-w64" `
        -lglfw3dll -lopengl32 -lglu32 -o build\museu3d.exe
    if ($LASTEXITCODE -ne 0) { throw 'Falha na compilacao.' }
    Copy-Item -LiteralPath "$glfwRoot\lib-mingw-w64\glfw3.dll" -Destination build\glfw3.dll
    New-Item -ItemType Directory -Force -Path build\assets | Out-Null
    Copy-Item -LiteralPath assets\earth.ppm -Destination build\assets\earth.ppm
    Copy-Item -LiteralPath assets\CREDITS.md,assets\THREEJS-LICENSE.txt -Destination build\assets
    Write-Host 'Compilado: build\museu3d.exe'
} finally {
    Pop-Location
}
