
script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source $script_dir/../.zephyr_venv/bin/activate

# ./scripts/checkpatch.pl --git HEAD-2
# ./scripts/ci/check_compliance.py

# Put your board under test to this list (default all boards)
board_list=(
	"nucleo_g474re/stm32g474xx"
	# "nucleo_h745zi_q/stm32h745xx/m7"
	# "nucleo_h745zi_q/stm32h745xx/m4"
)

# The apps are defined for this application
app_list=(
	# "tests/drivers/build_all/opamp"
	# "tests/drivers/opamp/opamp_api"
	"samples/drivers/opamp/output_measure"
)

# This array shall have same size as app_list
sleep_time_sec_list=(
	2
	2
	2
)

# for i in "${!app_list[@]}"; do
# 	west twister -T ${app_list[$i]} --vendor st
# done
# exit $?;

script_path="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# Iterate over boards:
for board in "${board_list[@]}"; do
	# Iterate over apps:
	for i in "${!app_list[@]}"; do
		app="${app_list[$i]}"
		app_abs_path="$script_path/$app"
		safe_board_name="${board//\//_}"
		build_dir="$app_abs_path/build/$safe_board_name"
		echo "--> use build directory: $build_dir"

		rm -rf $build_dir
		# west twister -p $board -T $app_abs_path --vendor st \
		# 	--device-testing --device-serial-baud 115200 --device-serial "/dev/ttyACM0" -v
		west build -p auto -d $build_dir -b $board $app_abs_path
		[ $? -ne 0 ] && exit $?;

		west flash -d $build_dir
		[ $? -ne 0 ] && exit $?;
		# echo "--> wait ${sleep_time_sec_list[$i]} sec. for $board $app to finish..."
		# sleep ${sleep_time_sec_list[$i]}
	done
done

exit 0;
