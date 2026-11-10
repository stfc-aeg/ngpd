echo "Copying Source files from SVN repo"

# useage function for help string
useage() {
    echo "Usage: $0 -d <SVN Repo Root> [-h for Help]"
}


OPTBIND=1

script=$(readlink -f "$0")
basedir=$(dirname $script)
echo "$basedir"

svn_root="/usr/lib/det-software"

# NGPD Libs
ngpd_src=("*.c")
headers=(
    "ngpd.h"
    "ngzmp.h"
    "ngzmp_dma_protocol.h"
    "axi_hist.h"
    "zynqmp_api.h"
    "zynqmp_protocol.h"
    "lmk0482x.h"
)

#imgmod lib
imgmod_src=(
    "img_mod_linux.c"
    "img_mod.c")
headers+=("datamod.h" "os9types.h")

while getopts "d:h" opt; do
    case $opt in
        d)
        svn_root="$OPTARG"
        ;;
        h)
        useage
        exit 0
        ;;
        \?)
        echo "Invalid Option: -$OPTARG"
        useage
        exit 1
        ;;
        :)
        echo "Option -$OPTARG requires an argument"
        useage
        exit 1
        ;;
    esac
done

ngpd_root="$svn_root/none_vme/ngpd/lib"
ngpd_cal_root="$svn_root/none_vme/ngpd/libcal"
zynqmp_root="$svn_root/none_vme/ngpd/libzynqmp"
imgmod_root="$svn_root/display/id/img_mod_lib"

include_path=("$svn_root/libs/include" "$svn_root/none_vme/ngpd/include")

echo "Copying NGPD Source from $svn_root"

# copying Header Files
for header in "${headers[@]}"
do
    x=($(find "${include_path[@]}" -name "$header"))
    echo "Copying $header"
    cp -f "$x" "$basedir/include"
done

# copying NGPD Lib Source Files
for src in "${ngpd_src[@]}"
do
    files=($(find "$ngpd_root" -name "$src"))
    for file in "${files[@]}"
    do
        echo "Copying $file"
        cp -f "$file" "$basedir/ngpd"
    done
done

# copying ngpd_cal Source Files
for src in "${ngpd_src[@]}"
do
    files=($(find "$ngpd_cal_root" -name "$src"))
    for file in "${files[@]}"
    do
        echo "Copying $file"
        cp -f "$file" "$basedir/ngpd_cal"
    done
done

# copying zynqmp Source Files
for src in "${ngpd_src[@]}"
do
    files=($(find "$zynqmp_root" -name "$src"))
    for file in "${files[@]}"
    do
        echo "Copying $file"
        cp -f "$file" "$basedir/zynqmp"
    done
done

# copying imgmod source files
for src in "${imgmod_src[@]}"
do
    files=($(find "$imgmod_root" -name "$src"))
    for file in "${files[@]}"
    do
        echo "Copying $file"
        cp -f "$file" "$basedir/img_mod"
    done
done

# getting SVN revision number
svn_rev=$(svn info --show-item revision "$svn_root")
echo "SVN REVISION: $svn_rev"
$(sed -i "s/SVN_VERSION [0-9]*/SVN_VERSION $svn_rev/" include/ngpd_version.h)


