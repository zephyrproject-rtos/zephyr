/*
 * Copyright 2019-2020, Synopsys, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-3-Clause license found in
 * the LICENSE file in the root directory of this source tree.
 *
 */

/*
 * LSTM Based NN Example for UCI Smartphones HAR Dataset
 *
 * Based on the project of Guillaume Chevalie:
 * https://github.com/guillaume-chevalier/LSTM-Human-Activity-Recognition
 *
 * Dataset info:
 * https://archive.ics.uci.edu/ml/datasets/human+activity+recognition+using+smartphones
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "mli_types.h"

#include "har_smartphone_model.h"
#include "har_smartphone_ref_inout.h"
#include "examples_aux.h"
#include "tensor_transform.h"
#include "tests_aux.h"

#if defined(__GNUC__) && !defined(__CCAC__)
extern int start_init(void);
#endif  /* if defined (__GNUC__) && !defined (__CCAC__) */

/* Root to referenc IR vectors for comparison */
/* pass "./ir_idx_300" for debug */
static const char kHarIrRefRoot[] = "";
static const char kOutFilePostfix[] = "_out";

static float kSingleInSeq[IN_POINTS] = IN_SEQ_300;
static float kSingleOutRef[OUT_POINTS] = OUT_SCORES_300;

static void har_smartphone_preprocessing(const void *raw_input_, mli_tensor *net_input_);

#define EXAMPLE_MAX_MODE (3)
int mode; /* emulation argc for GNU toolchain */
char param[EXAMPLE_MAX_MODE][256]; /* emulation argv for GNU toolchain */

/* -------------------------------------------------------------------------- */
/*                                    MAIN                                    */
/* -------------------------------------------------------------------------- */
int main(int argc, char **argv)
{
#if defined(__GNUC__) && !defined(__CCAC__)
	/* ARC GNU tools */
	/* fill mode and param from cmd line script before use */
#else
	/* Metaware tools */
	/* fill mode and param from argc and argv */
	if (argc <= EXAMPLE_MAX_MODE) {
		mode = argc;

		for (int i = 0; i < mode; i++) {
			memcpy(&param[i][0], argv[i], strlen(argv[i]));
		}
	}
#endif /* if defined (__GNUC__) && !defined (__CCAC__) */
/*please change mode=1,2,3 manually*/
mode = 1;
strcpy(param[0], "dummy_for_check");
strcpy(param[1], "small_test_base/tests.idx");
strcpy(param[2], "small_test_base/labels.idx");
	/* checking that variables are set */
	if (mode == 0) {
		printf("ERROR: mode not set up\n");
#if defined(__GNUC__) && !defined(__CCAC__)
		/* ARC GNU tools */
		printf("Please set up mode\n");
		printf("Please check that you use mdb_com_gnu script with correct setups\n");
#else
		/* Metaware tools */
		printf("App command line:\n"
		       "\t%s\n\t\tProcess single hardcoded vector\n\n"
		       "\t%s <input_test_base.idx>\n\t\tProcess testset from file and\n"
		       "\t\t output model results to <input_test_base.idx_out> file\n\n",
		       argv[0], argv[0]);
#endif /* if defined (__GNUC__) && !defined (__CCAC__) */
		return 2; /* Error: mode not set */
	}

	for (int i = 0; i < mode; i++) {
		if (param[i][0] == 0) {
			printf("param[%d][0] not set.\n", i);
			if (i == 0)
				printf("Please set up dummy string for check.\n");
			if (i == 1)
				printf("Please set up input IDX file.\n");
			if (i == 2)
				printf("Please set up labels IDX file.\n");
			return 2; /* Error: param not set */
		}
	}

	switch (mode) {
	/* No Arguments for app. Process single hardcoded input */
	/* Print various measures to stdout */
	case 1:
		printf("HARDCODED INPUT PROCESSING\n");
		model_run_single_in(kSingleInSeq, kSingleOutRef, har_smartphone_net_input,
				    har_smartphone_net_output, har_smartphone_preprocessing,
				    har_smartphone_net, kHarIrRefRoot);
		break;

	/* APP <input_test_base.idx> */
	/* Output vectors will be written to <input_test_base.idx_out> file */
	case 2:
		printf("Input IDX testset to output IDX set\n");
		char *out_path = malloc(strlen(param[1]) + strlen(kOutFilePostfix) + 1);

		if (out_path == NULL) {
			printf("mem allocation failed\n");
			break;
		}
		out_path[0] = 0;
		strcat(out_path, param[1]);
		strcat(out_path, kOutFilePostfix);

		model_run_idx_base_to_idx_out(param[1], out_path, har_smartphone_net_input,
					      har_smartphone_net_output,
					      har_smartphone_preprocessing, har_smartphone_net,
					      NULL);
		free(out_path);
		break;

	/* APP <input_test_base.idx> <input_test_labels.idx> */
	/* Calculate accuracy of the model */
	case 3:
		printf("ACCURACY CALCULATION on Input IDX testset according to IDX labels set\n");
		model_run_acc_on_idx_base(param[1], param[2], har_smartphone_net_input,
					  har_smartphone_net_output, har_smartphone_preprocessing,
					  har_smartphone_net, NULL);
		break;

	/* Unknown format */
	default:
		printf("App command line:\n"
		       "\t%s\n\t\tProcess single hardcoded vector\n\n"
		       "\t%s <input_test_base.idx>\n\t\tProcess testset from file and\n"
		       "\t\t output model results to <input_test_base.idx_out> file\n\n",
		       argv[0], argv[0]);
		break;
	}
	printf("FINISHED\n");

	return 0;
}

/* -------------------------------------------------------------------------- */
/*                    Other internal functions and routines                   */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/*                 Data pre-processing for HAR Smartphone net                 */
/* -------------------------------------------------------------------------- */
static void har_smartphone_preprocessing(const void *raw_input_, mli_tensor *net_input_)
{
	const float *in = raw_input_;

	mli_hlp_float_to_fx_tensor(in, IN_POINTS, net_input_);
}
