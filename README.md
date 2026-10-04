# NieR: Automata — Auto Chips on Any Difficulty

Plugin for the Steam version of NieR: Automata (17 July 2021). Auto-Attack, Auto-Fire, Auto-Evade, Auto-Program and Auto-Weapon Switch work on Normal, Hard and Very Hard. Enemy health and damage stay on the difficulty selected in the menu.

The plugin patches two checks in the running game. `NieRAutomata.exe` on disk is left as Steam installed it. Cheat Engine is not required.

The five chips use IDs `0x0D1A`–`0x0D1E`. Each check is `cmp eax, 4` / `ja`. The plugin changes that `ja` (`0x77`) into `jmp` (`0xEB`) so the chips take the same path as every other chip. The difficulty value itself is read from many other places and is not written.

Auto-use Item on Very Hard is a separate check and stays unchanged.

## Install

The game's mod loader (the one NAMH ships, and [xxk-i/nier-mod-loader](https://github.com/xxk-i/nier-mod-loader)) waits until the working directory is the `data` folder, then loads `mods\plugins\*.dll` from there. Files belong here:

```text
NieRAutomata\data\mods\plugins\AutoChips.dll
NieRAutomata\data\mods\config.ini
```

`config.ini` needs this under `[DLL]`:

```ini
AutoChips=EARLY
```

Special K has to load `modloader.dll`. A NAMH install with the ModLoader wrapper already does that. Do not replace `dinput8.dll`, `dxgi.dll` or `modloader.dll`.

### NAMH

1. Open the Mods tab.
2. Add `dist/NieRAutomata-AutoChips-1.0.zip` and press Apply.
3. If NAMH asks to install the DLL through modloader, confirm.
4. Launch the game. `data\mods\plugins\AutoChips.log` should contain `patched` for both sites.

### By hand

Copy the `data` folder from the zip into the game directory and merge. If `data\mods\config.ini` already exists, do not replace it. Add the `AutoChips=EARLY` line under `[DLL]`.

### Remove

Delete `data\mods\plugins\AutoChips.dll` and the `AutoChips` line in `data\mods\config.ini`.

## In game

Set the difficulty above Easy, equip an auto-control chip, and hold the auto-control button. Buy the chips from the Resistance Camp supply trader if they are not in the inventory.

## Build

The source is `src/autochips.c`. On Windows, [LLVM MinGW](https://github.com/mstorsjo/llvm-mingw/releases) (`llvm-mingw-*-ucrt-x86_64.zip`) unpacked to `tools/llvm-mingw` is enough. Visual Studio is not required.

```bat
build.bat
```

That writes `build\AutoChips.dll` and `build\check_patterns.exe`. The checker confirms both byte patterns exist once in an unpacked exe:

```bat
build\check_patterns.exe "D:\SteamLibrary\steamapps\common\NieRAutomata\NieRAutomata.exe"
```

`build.bat` looks for `tools\llvm-mingw\bin\clang.exe`, then `tools\llvm-mingw-*\bin\clang.exe`, then `clang` on `PATH`.

## Layout

```text
src/autochips.c          plugin
build.bat                build with clang
package/                 files that go into the game
dist/NieRAutomata-AutoChips-1.0.zip
```

## License

[MIT](LICENSE).

---

# NieR: Automata — чипы авто-контроля на любой сложности

Плагин для Steam-версии от 17 июля 2021. Автоатака, автовыстрел, автоуклонение, автопрограмма и автосмена оружия работают на обычной, высокой и очень высокой сложности. Здоровье и урон врагов остаются от выбранной сложности.

Правка делается в памяти запущенной игры. Файл `NieRAutomata.exe` на диске не меняется. Cheat Engine не нужен.

Пять чипов имеют ID `0x0D1A`–`0x0D1E`. В двух функциях стоит `cmp eax, 4` / `ja`: если это один из этих чипов и сложность не лёгкая, чип запрещается. Плагин меняет `ja` (`0x77`) на `jmp` (`0xEB`), и чип идёт по обычному пути. Само значение сложности не записывается: его читают десятки других мест.

Запрет чипа авто-предметов на очень высокой сложности — отдельная проверка, мод её не трогает.

## Установка

Загрузчик модов, который ставит NAMH, и [xxk-i/nier-mod-loader](https://github.com/xxk-i/nier-mod-loader) ждут, пока рабочая папка станет `data`, и затем грузят `mods\plugins\*.dll` уже оттуда. Файлы лежат так:

```text
NieRAutomata\data\mods\plugins\AutoChips.dll
NieRAutomata\data\mods\config.ini
```

В `config.ini` в секции `[DLL]` нужна строка:

```ini
AutoChips=EARLY
```

Special K должен загружать `modloader.dll`. Установка NAMH с обёрткой ModLoader это уже делает. `dinput8.dll`, `dxgi.dll` и `modloader.dll` заменять не нужно.

### NAMH

1. Вкладка Mods.
2. Добавить `dist/NieRAutomata-AutoChips-1.0.zip` и нажать Apply.
3. Если NAMH спросит, ставить ли DLL через modloader, подтвердить.
4. Запустить игру. В `data\mods\plugins\AutoChips.log` у обоих сайтов должно быть `patched`.

### Вручную

Скопировать папку `data` из архива в каталог игры, с объединением. Если `data\mods\config.ini` уже есть, не заменять его, а дописать `AutoChips=EARLY` в секцию `[DLL]`.

### Удаление

Удалить `data\mods\plugins\AutoChips.dll` и строку `AutoChips` из `data\mods\config.ini`.

## В игре

Поставить сложность выше лёгкой, экипировать чип авто-контроля и удерживать кнопку авто-контроля. Если чипов нет в инвентаре, их продаёт торговец припасами в лагере сопротивления.

## Сборка

Исходник — `src/autochips.c`. На Windows достаточно [LLVM MinGW](https://github.com/mstorsjo/llvm-mingw/releases) (`llvm-mingw-*-ucrt-x86_64.zip`), распакованного в `tools/llvm-mingw`. Visual Studio не нужен.

```bat
build.bat
```

Появятся `build\AutoChips.dll` и `build\check_patterns.exe`. Проверка ищет оба шаблона в exe:

```bat
build\check_patterns.exe "D:\SteamLibrary\steamapps\common\NieRAutomata\NieRAutomata.exe"
```

`build.bat` берёт `tools\llvm-mingw\bin\clang.exe`, иначе `tools\llvm-mingw-*\bin\clang.exe`, иначе `clang` из `PATH`.
