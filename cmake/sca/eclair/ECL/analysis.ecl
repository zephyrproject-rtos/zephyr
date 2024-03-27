-setq=data_dir,getenv("ECLAIR_DATA_DIR")
-setq=set,getenv("ECLAIR_RULES_SET")

-enable=B.REPORT.ECB
-config=B.REPORT.ECB,output=join_paths(data_dir,"FRAME.@FRAME@.ecb")
-config=B.REPORT.ECB,preprocessed=show
-config=B.REPORT.ECB,macros=10

-enable=B.EXPLAIN

-eval_file=common_config.ecl
