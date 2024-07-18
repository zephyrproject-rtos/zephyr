//! DTS Parser
//!
//! Parse a limited subset of the devicetree source file that is output by the device tree compiler.
//! This is used to parse the `zephyr.dts` file generated as a part of a Zephyr build.

use std::{collections::BTreeMap, rc::Rc};

use pest::{iterators::{Pair, Pairs}, Parser};
use pest_derive::Parser;

use crate::devicetree::Phandle;

use super::{ordmap::OrdMap, DeviceTree, Node, Property, Value, Word};

#[derive(Parser)]
#[grammar = "devicetree/dts.pest"]
pub struct Dts;

pub fn parse(text: &str, ords: &OrdMap) -> DeviceTree {
    let pairs = Dts::parse(Rule::file, text)
        .expect("Parsing zephyr.dts");

    let b = TreeBuilder::new(ords);
    b.walk(pairs)
}

struct TreeBuilder<'a> {
    ords: &'a OrdMap,
    /// All labels.
    labels: BTreeMap<String, Rc<Node>>,
}

impl<'a> TreeBuilder<'a> {
    fn new(ords: &'a OrdMap) -> TreeBuilder<'a> {
        TreeBuilder {
            ords,
            labels: BTreeMap::new(),
        }
    }

    fn walk(mut self, pairs: Pairs<'_, Rule>) -> DeviceTree {
        // There is a single node at the top.
        let node = pairs.into_iter().next().unwrap();
        assert_eq!(node.as_rule(), Rule::node);

        DeviceTree {
            root: self.walk_node(node, "", &[]),
            labels: self.labels,
        }
    }

    // This is a single node in the DTS.  The name should match one of the ordmap entries.
    // The root node doesn't get a nodename.
    fn walk_node(&mut self, node: Pair<'_, Rule>, path: &str, route: &[String]) -> Rc<Node> {
        /*
        let ord = self.ords.0.get(name)
            .expect("Unexpected node path");
        println!("Root: {:?} {}", name, ord);
        */

        let mut name = LazyName::new(path, route.to_owned(), &self.ords);
        let mut labels = Vec::new();
        let mut properties = Vec::new();
        let mut children = Vec::new();

        for pair in node.into_inner() {
            match pair.as_rule() {
                Rule::nodename => {
                    let text = pair.as_str();
                    name.set(text.to_string());
                }
                Rule::label => {
                    labels.push(pair.as_str().to_string());
                }
                Rule::property => {
                    properties.push(decode_property(pair));
                }
                Rule::node => {
                    let child_path = name.path_ref();
                    children.push(self.walk_node(pair, child_path, &name.route_ref()));
                }
                r => panic!("node: {:?}", r),
            }
        }

        // Make a clone of the labels, as we need them cloned anyway.
        let labels2 = labels.clone();

        // Build this node.
        println!("Node: {:?}", name.path_ref());
        let mut result = name.into_node();
        result.labels = labels;
        result.properties = properties;
        result.children = children;
        let node = Rc::new(result);

        // Insert all of the labels.
        for lab in labels2 {
            self.labels.insert(lab, node.clone());
        }
        node
    }
}

/// Decode a property node in the parse tree.
fn decode_property(node: Pair<'_, Rule>) -> Property {
    let mut name = None;
    let mut value = Vec::new();
    for pair in node.into_inner() {
        match pair.as_rule() {
            Rule::nodename => {
                name = Some(pair.as_str().to_string());
            }
            Rule::words => {
                value.push(Value::Words(decode_words(pair)));
            }
            Rule::phandle => {
                // TODO: Decode these.
                println!("phandle: {:?}", pair.as_str());
                value.push(Value::Phandle(Phandle::new(pair.as_str()[1..].to_string())));
            }
            Rule::string => {
                // No escapes at this point.
                let text = pair.as_str();
                // Remove the quotes.
                let text = &text[1..text.len()-1];
                value.push(Value::String(text.to_string()));
            }
            Rule::bytes => {
                value.push(Value::Bytes(decode_bytes(pair)));
            }
            r => panic!("rule: {:?}", r),
        }
    }
    Property { name: name.unwrap(), value }
}

fn decode_words<'i>(node: Pair<'i, Rule>) -> Vec<Word> {
    let mut value = Vec::new();
    for pair in node.into_inner() {
        match pair.as_rule() {
            Rule::hex_number => {
                let text = pair.as_str();
                let num = u32::from_str_radix(&text[2..], 16).unwrap();
                value.push(Word::Number(num));
            }
            Rule::decimal_number => {
                let text = pair.as_str();
                let num = u32::from_str_radix(text, 10).unwrap();
                value.push(Word::Number(num));
            }
            Rule::phandle => {
                println!("phandle: {:?}", pair.as_str());
                let text = pair.as_str();
                value.push(Word::Phandle(Phandle::new(text[1..].to_string())));
            }
            _ => unreachable!(),
        }
    }
    value
}

fn decode_bytes<'i>(node: Pair<'i, Rule>) -> Vec<u8> {
    let mut value = Vec::new();
    for pair in node.into_inner() {
        match pair.as_rule() {
            Rule::plain_hex_number => {
                let text = pair.as_str();
                let num = u8::from_str_radix(text, 16).unwrap();
                value.push(num)
            }
            _ => unreachable!(),
        }
    }
    value
}

// Lazily track the path and node name.  The parse tree has the nodename for a given node come after
// entering the node, but before child nodes are seen.
struct LazyName<'a, 'b> {
    // The parent path leading up to this node.  Will be the empty string for the root node.
    path: &'a str,
    route: Vec<String>,
    ords: &'b OrdMap,
    // Our information, once we have it.
    info: Option<Info>,
}

struct Info {
    name: String,
    // Our path, the parent path combined with our name.
    path: String,
    ord: usize,
}

impl<'a, 'b> LazyName<'a, 'b> {
    fn new(path: &'a str, route: Vec<String>, ords: &'b OrdMap) -> LazyName<'a, 'b> {
        if path.is_empty() {
            let ord = ords.0["/"];
            LazyName {
                path,
                route,
                ords,
                info: Some(Info {
                    name: "/".to_string(),
                    path: "/".to_string(),
                    ord,
                })
            }
        } else {
            LazyName {
                path,
                route,
                ords,
                info: None,
            }
        }
    }

    /// Indicate that we now know our name.
    fn set(&mut self, name: String) {
        if self.info.is_some() {
            panic!("Grammar error, node has multiple names");
        }

        self.route.push(name.clone());

        let mut path = self.path.to_string();
        if path.len() > 1 {
            path.push('/');
        }
        path.push_str(&name);
        println!("node: {:?}", path);
        let ord = self.ords.0[&path];
        self.info = Some(Info {
            name,
            path,
            ord,
        });
    }

    fn path_ref(&self) -> &str {
        &self.info.as_ref().unwrap().path
    }

    fn route_ref(&self) -> &[String] {
        &self.route
    }

    fn ord(&self) -> usize {
        self.info.as_ref().unwrap().ord
    }

    // Turn this into a template for a node, with the properties, labels and children as empty.
    fn into_node(self) -> Node {
        let info = self.info.unwrap();

        Node {
            name: info.name,
            path: info.path,
            route: self.route,
            ord: info.ord,
            labels: Vec::new(),
            properties: Vec::new(),
            children: Vec::new(),
        }
    }
}
