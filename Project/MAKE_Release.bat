set ArchiveName=GeneralsBigCreator_v1.2

set ReleaseDir=Release
set ReleaseUnpackedDir=ReleaseUnpacked

if not exist %ReleaseDir% mkdir %ReleaseDir%
if not exist %ReleaseUnpackedDir% mkdir %ReleaseUnpackedDir%

xcopy /Y Code\vc9\Release\*.exe %ReleaseUnpackedDir%\

tar.exe -a -c -C %ReleaseUnpackedDir% -f %ReleaseDir%\%ArchiveName%.zip *.*
