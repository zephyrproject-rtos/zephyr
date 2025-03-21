script_dir=$(dirname $(realpath $0))

function gen_pte(){
    echo "Generating PTE file for model..."
    python ${script_dir}/model/gen_pte.py --pte ${script_dir}/model/model.pte
    # export pte to header file
    python ${script_dir}/model/pte_to_header.py --pte ${script_dir}/model/model.pte --outfile ${script_dir}/executor_runner/model_pte.c

    echo "Generated ${script_dir}/model/model.pte"
    echo "Generated ${script_dir}/executor_runner/model_pte.c"
}

gen_pte