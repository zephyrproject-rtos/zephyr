//! Devicetree ordmap
//!
//! The OrdMap provides a mapping between nodes on the devicetree, and their "ord" index.

use std::{collections::BTreeMap, fs::File, io::{BufRead, BufReader}, path::Path, str::FromStr};

use regex::Regex;

pub struct OrdMap(pub BTreeMap<String, usize>);

impl OrdMap {
    pub fn new<P: AsRef<Path>>(path: P) -> OrdMap {
        let mut result = BTreeMap::new();

        let path_re = Regex::new(r#"^#define DT_(.*)_PATH "(.*)"$"#).unwrap();
        let ord_re = Regex::new(r#"^#define DT_(.*)_ORD (.*)$"#).unwrap();

        // The last C name seen.
        let mut c_name = "".to_string();
        let mut dt_path = "".to_string();

        let fd = File::open(path)
            .expect("Opening devicetree_generated.h");
        for line in BufReader::new(fd).lines() {
            let line = line.expect("Reading from devicetree_generated.h");

            if let Some(caps) = path_re.captures(&line) {
                // println!("Path: {:?} => {:?}", &caps[1], &caps[2]);
                c_name = caps[1].to_string();
                dt_path = caps[2].to_string();
            } else if let Some(caps) = ord_re.captures(&line) {
                // println!("Ord: {:?} => {:?}", &caps[1], &caps[2]);
                let ord = usize::from_str(&caps[2]).unwrap();
                assert_eq!(caps[1].to_string(), c_name);
                result.insert(dt_path.clone(), ord);
            }
        }

        OrdMap(result)
    }
}
