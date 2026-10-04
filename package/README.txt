NieR: Automata — Auto Chips on Any Difficulty
==============================================

Lets Auto-Attack, Auto-Fire, Auto-Evade, Auto-Program and Auto-Weapon Switch
work on Normal, Hard and Very Hard. Combat difficulty is unchanged.
NieRAutomata.exe on disk is not modified.

Requires the Steam build from 17 July 2021 and nier-mod-loader (the copy
NAMH installs is enough). Special K must load modloader.dll, which NAMH
already sets up when the ModLoader wrapper is enabled.

Install with NAMH
-----------------
1. Open NAMH, Mods tab.
2. Add this zip and press Apply.
3. Confirm Install (modloader) if NAMH asks.
4. Start the game. After you reach gameplay, data\mods\plugins\AutoChips.log
   should contain "patched".

The loader reads files only after the game sets its working directory to
the data folder, so the plugin lives at:

  data\mods\plugins\AutoChips.dll
  data\mods\config.ini

Install by hand
---------------
Copy the data folder from this zip into the NieR: Automata directory and
merge folders. If data\mods\config.ini already exists, do not replace it.
Open it and add this line under [DLL]:

  AutoChips=EARLY

Remove
------
Delete data\mods\plugins\AutoChips.dll and the AutoChips line in
data\mods\config.ini.

In game
-------
Set difficulty above Easy, equip an auto-control chip, and hold the
auto-control button. The "auto chips" line should switch on. Buy the chips
from the Resistance Camp supply trader if they are not in your inventory yet.

Русский
-------
Мод разрешает пять чипов авто-контроля на любой сложности. Урон и здоровье
врагов остаются от выбранной сложности. Файл NieRAutomata.exe не меняется.

Установка через NAMH: вкладка Mods, добавить этот zip, Apply. Если NAMH
спросит Install (modloader), подтвердить.

Вручную: скопировать папку data из архива в каталог игры. Если
data\mods\config.ini уже есть, не заменять его, а дописать в секцию [DLL]
строку AutoChips=EARLY.

Удаление: убрать data\mods\plugins\AutoChips.dll и строку AutoChips из
config.ini.

В игре сложность выше лёгкой, чип авто-контроля в слоте, удержать кнопку
авто-контроля. Лог: data\mods\plugins\AutoChips.log, в нём должно быть
patched.
