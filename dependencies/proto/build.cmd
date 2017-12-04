@echo off
echo Compiling protocol buffers...
for %%i in (*.proto) do tools\protoc --error_format=msvs --cpp_out=c++ %%i
for %%i in (*.proto) do tools\protoc --go_out=golang/ %%i
for %%i in (*.proto) do tools\protogen -i:%%i -t:csharp -ns:NPx -o:csharp/%%i.cs

tools\protoc --error_format=msvs --descriptor_set_out=proto.desc auth.proto friends.proto hello.proto servers.proto storage.proto

echo Copying protocol buffers...
copy /y "c++\*.pb.cc" "..\..\src"
copy /y "c++\*.pb.h" "..\..\src"

REM copy /y csharp\* ..\server\NPServer\Protocol\
REM copy /y csharp\* ..\..\plaza\client\Plaza\Protocol\

REM copy /y proto.desc ..\npm\server\
echo Generating message definition...

tools\php\php.exe .\tools\generate-code.php
pause 0