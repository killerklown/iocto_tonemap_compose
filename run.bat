%cd%/bin/Release/tonemap.exe --no-srgb -o %cd%/out/tm_night_linear_e0.png %cd%/in/tm_night.hdr
%cd%/bin/Release/tonemap.exe -o %cd%/out/tm_night_srgb_e0.png %cd%/in/tm_night.hdr
%cd%/bin/Release/tonemap.exe --filmic -o %cd%/out/tm_night_filmic_e0.png %cd%/in/tm_night.hdr
%cd%/bin/Release/tonemap.exe --filmic -e 2 -o %cd%/out/tm_night_filmic_ep2.png %cd%/in/tm_night.hdr

%cd%/bin/Release/compose.exe -o %cd%/out/comp_gridcheck.png %cd%/in/comp_grid.png %cd%/in/comp_check.png
%cd%/bin/Release/compose.exe -o %cd%/out/comp_gridramp.png %cd%/in/comp_grid.png %cd%/in/comp_ramp.png
%cd%/bin/Release/compose.exe -o %cd%/out/comp_gridrampcheck.png %cd%/in/comp_grid.png %cd%/in/comp_ramp.png %cd%/in/comp_check.png
%cd%/bin/Release/compose.exe -o %cd%/out/comp_seaman.png %cd%/in/comp_sea.png %cd%/in/comp_man.png
