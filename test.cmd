@echo off
del go.cmd
echo echo on >go.cmd
cd tmp
for %%f in (*.h) do echo hdrstrip tmp\%%f -b >> ..\go.cmd
cd ..
go
