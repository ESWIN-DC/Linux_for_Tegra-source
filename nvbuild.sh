#!/bin/bash

# Copyright (c) 2019-2020, NVIDIA CORPORATION. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#  * Neither the name of NVIDIA CORPORATION nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# This script builds trusty sources in this directory.
# Usage: ./${SCRIPT_NAME}.sh [OPTIONS]
set -e

# shellcheck disable=SC2046
SCRIPT_DIR="$(dirname $(readlink -f "${0}"))"
SCRIPT_NAME="$(basename "${0}")"

source "${SCRIPT_DIR}/nvcommon_build.sh"

function usage {
        cat <<EOM
Usage: ./${SCRIPT_NAME} [OPTIONS]
This script builds trusty sources in this directory.
It supports following options.
OPTIONS:
        -h                  Displays this help
        -b <board_name>     Target board
                            e.g. -b t186ref
EOM
}

function check_pre_req {
	check_vars "NV_TARGET_BOARD" "CROSS_COMPILE_ARM_PATH"
	CROSS_COMPILE_ARM="${CROSS_COMPILE_ARM_PATH}/bin/arm-linux-gnueabihf-"

	if [ ! -f "${CROSS_COMPILE_ARM}gcc" ]; then
		echo " path ${CROSS_COMPILE_ARM}gcc does not exist"
		exit 1
	fi
}

# parse input parameters
function parse_input_param {
	while [ $# -gt 0 ]; do
		case ${1} in
			-h)
				usage
				exit 0
				;;
			-b)
				NV_TARGET_BOARD="${2}"
				shift 2
				;;
			*)
				echo "Error: Invalid option ${1}"
				usage
				exit 1
				;;
			esac
	done
}

function build_trusty_sources {
	echo "Building trusty sources .."

	# execute building steps
	local source_dir="${SCRIPT_DIR}/trusty/"
	local make_target=""
	local target_socs=()
	if [ "${NV_TARGET_BOARD}" = "t210ref" ]; then
		make_target="t210"
		target_socs=("t210")
	elif [ "${NV_TARGET_BOARD}" = "t186ref" ]; then
		make_target="t186"
		target_socs=("t186" "t194")
	else
		echo "${FUNCNAME[0]}: ${NV_TARGET_BOARD} is not supported"
		return 0
	fi

	for target_soc in "${target_socs[@]}"; do
		# shellcheck disable=SC2154
		echo "Building sources in ${src_pkg} for ${make_target}/${target_soc}"

		"${MAKE_BIN}" -C "${source_dir}" \
			"${make_target}" \
			PROJECT="${make_target}" \
			TARGET="${make_target}" \
			TARGET_SOC="${target_soc}" \
			BUILDROOT="./${make_target}-${target_soc}" \
			TOOLCHAIN_PREFIX="${CROSS_COMPILE_AARCH64}" \
			ARCH_arm_TOOLCHAIN_PREFIX="${CROSS_COMPILE_ARM}" \
			ARCH_arm64_TOOLCHAIN_PREFIX="${CROSS_COMPILE_AARCH64}" \
			DEBUG=0 DEBUG_LVL=0 DEFAULT_OTE_APP_DEBUGLEVEL=1 NOECHO=@ \
			TRUSTY_VARIANT=l4t TRUSTY_MULTI_GUEST_CONFIGURATION= \
			-j"${NPROC}" --output-sync=target

		bin="${source_dir}/${make_target}-${target_soc}/build-${make_target}/lk.bin"
		if [ ! -f "${bin}" ]; then
			echo "Error: Missing output binary ${bin}"
			exit 1
		fi
	done

	echo "Trusty sources compiled successfully."
}

# shellcheck disable=SC2068
parse_input_param $@
check_pre_req
build_trusty_sources
