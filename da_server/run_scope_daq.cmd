file_name="scope.d16"
if %nargs>=1 "file_name=%1"
ngpd scope set-options 0 0 set-streams 1
ngpd scope setup0-4 0 0 0 0 inp
set-func "scope_path" "ngpd scope open 0" local
set-func "num_x" "unif-get-nx %scope_path" local
set-func "num_y" "unif-get-ny %scope_path" local
set-func "num_t" "unif-get-nt %scope_path" local
#file get-next-index %data_dir "NGPD Scope Mode" %comment1 "Shape=("+(string)num_x+", "+(string)%num_y+", "+(string)num_t+")" var "seq"
#for "pass" 0 %num_pass-1 local
@scope
read 0 0 0 %num_x %num_y %num_t from %scope_path to-local-file %file_name raw
#next
close %scope_path