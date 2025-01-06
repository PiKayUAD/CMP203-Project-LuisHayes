echo on
:: For each file in the directory, convert each texture format to a the .gnf
:: 
:: /r  - User specified root directory
:: %%a - Expands to the current file within the directory
:: *.  - Masks all files with the specified extension
::
 
for /r %2GameApp\assets %%a in (*.dds,*.png,*jpeg,*jpg,*.tga,*.bmp) do (
%1\host_tools\bin\image2gnf.exe -g 1 -i %%a -f R32G32B32A32Float -o %3assets\%%~na.gnf
)
::
:: This script is executed upon rebuilding the GameApp project.