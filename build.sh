
script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source $script_dir/../.zephyr_venv/bin/activate

# ./scripts/checkpatch.pl --git HEAD-2

# Put your board under test to this list (default all boards)
board_list=(
	# "nucleo_u031r8/stm32u031xx"
	# "nucleo_f030r8/stm32f030x8"
	# "nucleo_f070rb/stm32f070xb"
	# "nucleo_f072rb/stm32f072xb"
	# "nucleo_f091rc/stm32f091xc"
	# "nucleo_f103rb/stm32f103xb"
	# "nucleo_f207zg/stm32f207xx"
	# "nucleo_f302r8/stm32f302x8"
	# "nucleo_f303re/stm32f303xe"
	# "nucleo_f334r8/stm32f334x8"
	# "nucleo_f401re/stm32f401xe"
	# "nucleo_f410rb/stm32f410rx"
	# "nucleo_f411re/stm32f411xe"
	# "nucleo_f413zh/stm32f413xx"
	# "nucleo_f429zi/stm32f429xx"
	# "nucleo_f446re/stm32f446xx"
	# "nucleo_f746zg/stm32f746xx"
	# "nucleo_f756zg/stm32f756xx"
	# "nucleo_g071rb/stm32g071xx"
	# "nucleo_g431rb/stm32g431xx"
	# "nucleo_g474re/stm32g474xx"
	# "nucleo_h743zi/stm32h743xx"
	# "nucleo_c031c6/stm32c031xx"
	"nucleo_h745zi_q/stm32h745xx/m7"
	# "nucleo_h745zi_q/stm32h745xx/m4"
	# "nucleo_l053r8/stm32l053xx"
	# "nucleo_l073rz/stm32l073xx"
	# "nucleo_l152re/stm32l152xe"
	# "nucleo_l432kc/stm32l432xx"
	# "nucleo_l476rg/stm32l476xx"
	# "nucleo_l496zg/stm32l496xx"
	# "nucleo_u575zi_q/stm32u575xx"
	# "b_l072z_lrwan1/stm32l072xx"
	# "b_u585i_iot02a/stm32u585xx"
	# "stm32l562e_dk/stm32l562xx"
	# "stm32u5a9j_dk/stm32u5a9xx"
	# "stm32373c_eval/stm32f373xc"
	# "stm32f429i_disc1/stm32f429xx"
	# "stm32f746g_disco/stm32f746xx"
	# "stm32g0316_disco/stm32g031xx"
	# "stm32h735g_disco/stm32h735xx"
	# "stm32l476g_disco/stm32l476xx"
	# "stm32l496g_disco/stm32l496xx"
	# "stm32u5a9j_dk/stm32u5a9xx"
	# "disco_l475_iot1/stm32l475xx"
	# "nucleo_wb55rg/stm32wb55xx"
	# "stm32mp157c_dk2/stm32mp157cxx"
	# "stm32mp135f_dk/stm32mp135fxx"
)

# The apps are defined for this application
app_list=(
	"tests/drivers/comparator/gpio_loopback"
)

# This array shall have same size as app_list
sleep_time_sec_list=(
	1
	20
	60
)

# west twister -T "tests/drivers/interrupt_controller/intc_exti_stm32" --vendor st; exit $?

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
		echo "--> wait ${sleep_time_sec_list[$i]} sec. for $board $app to finish..."
		sleep ${sleep_time_sec_list[$i]}
	done
done

exit 0;
