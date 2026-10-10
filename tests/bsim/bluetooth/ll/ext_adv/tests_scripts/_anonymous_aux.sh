#!/usr/bin/env bash
# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

# The Controller does not send anonymous advertising with AD data, which has
# no AdvA in its AUX_ADV_IND either, so a playback device sends it from a Tx
# dump of the Phy that these functions write.

# The CRC of an advertising channel PDU given as hex octets
adv_crc() {
  # The CRC init of advertising channel PDUs, 0x555555, with its bits
  # reversed for this shift register, which takes the LSB of each octet first
  local state=$((0xAAAAAA))
  local octet bit i

  for octet in "$@"; do
    octet=$((16#${octet}))
    for ((i = 0; i < 8; i++)); do
      bit=$(((state ^ octet) & 1))
      octet=$((octet >> 1))
      state=$((state >> 1))
      if [ ${bit} -ne 0 ]; then
        state=$(((state | 0x800000) ^ 0x5A6000))
      fi
    done
  done

  printf "%02X %02X %02X" $((state & 0xFF)) $(((state >> 8) & 0xFF)) $((state >> 16))
}

# A PDU sent on the 1M PHY at the given time in us and frequency in MHz above
# 2400 MHz, with its preamble and access address of 5 octets before it
pdu_tx() {
  local start=$1
  local freq=$2
  shift 2
  local octets=("$@" $(adv_crc "$@"))
  local end=$((start + ((5 + ${#octets[@]}) * 8) - 1))
  local never=18446744073709551615

  echo -n "${start},${end},${freq}.000000,0x8E89BED6,16,0.000000,${never},${never},"
  echo "${#octets[@]},${octets[*]}"
}

# An extended advertising event with the given ADI: ADV_EXT_INDs on channels
# 37, 38 and 39 (2402, 2426 and 2480 MHz) 300 us apart, with an AuxPtr to the
# given AUX_ADV_IND 1.5 ms after the first one on channel 5 (2414 MHz). The
# AuxPtr has CA set and the offset in 30 us units.
aux_event() {
  local start=$1
  local adi=$2
  local aux_adv_ind=$3
  local freqs=(2 26 80)
  local offset i

  for i in 0 1 2; do
    offset=$(printf "%02X" $(((1500 - (i * 300)) / 30)))
    pdu_tx $((start + (i * 300))) ${freqs[i]} 07 07 06 18 ${adi} 45 ${offset} 00
  done

  pdu_tx $((start + 1500)) 14 ${aux_adv_ind}
}

# Writes <file>.Tx.csv: every 100 ms for 3 s, anonymous advertising with an
# AUX_ADV_IND, undirected with the AD data 01, and directed to the given random
# TargetA with the AD data 02
anonymous_aux_tx() {
  local file=$1
  local target_a=$2
  local t

  mkdir -p "$(dirname "${file}")"
  {
    echo -n "start_time,end_time,center_freq,phy_address,modulation,power_level,"
    echo "abort_time,recheck_time,packet_size,packet"
    for ((t = 50000; t < 3000000; t += 100000)); do
      aux_event ${t} "01 10" "07 07 03 08 01 10 02 FF 01"
      aux_event $((t + 50000)) "01 20" "87 0D 09 0A ${target_a} 01 20 02 FF 02"
    done
  } > "${file}.Tx.csv"
}
