script_dir=$(dirname $(realpath $0))

function gen_pte(){
    echo "Generating PTE file for model..."
    # Forward any additional arguments ($@) to the gen_pte.py script.
    python ${script_dir}/model/gen_pte.py --pte ${script_dir}/model/model.pte "$@"
    
    # Export the generated PTE to a header file.
    python ${script_dir}/model/pte_to_header.py --pte ${script_dir}/model/model.pte --outfile ${script_dir}/executor_runner/model_pte.c

    echo "Generated ${script_dir}/model/model.pte"
    echo "Generated ${script_dir}/executor_runner/model_pte.c"
}

# Pass all command-line arguments to the function.
gen_pte "$@"
