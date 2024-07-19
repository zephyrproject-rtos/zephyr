//! Support for augmenting the device tree.

use proc_macro2::TokenStream;
use quote::{format_ident, quote};

use super::{Node, Word};

/// This action is given to each node in the device tree, and it is given a chance to return
/// additional code to be included in the module associated with that entry.  These are all
/// assembled together and included in the final generated devicetree.rs.
pub trait Augment {
    /// The default implementation checks if this node matches and calls a generator if it does, or
    /// does nothing if not.
    fn augment(&self, node: &Node) -> TokenStream {
        if self.is_compatible(node) {
            self.generate(node)
        } else {
            TokenStream::new()
        }
    }

    /// A query if this node is compatible with this augment.  A simple case might check the node's
    /// compatible field, but also makes sense to check a parent's compatible.
    fn is_compatible(&self, node: &Node) -> bool;

    /// A generator to be called when we are compatible.
    fn generate(&self, node: &Node) -> TokenStream;
}

/// Return all of the Augments we wish to apply.
pub fn get_augments() -> Vec<Box<dyn Augment>> {
    vec![
        Box::new(GpioAugment),
        Box::new(GpioLedsAugment),
    ]
}

/// Augment gpio nodes.  Gpio nodes are indicated by the 'gpio-controller' property, and not a
/// particular compatible.
struct GpioAugment;

impl Augment for GpioAugment {
    fn is_compatible(&self, node: &Node) -> bool {
        node.has_prop("gpio-controller")
    }

    fn generate(&self, node: &Node) -> TokenStream {
        let ord = node.ord;
        let device = format_ident!("__device_dts_ord_{}", ord);
        quote! {
            pub unsafe fn get_instance() -> *const crate::sys::device {
                extern "C" {
                    static #device: crate::sys::device;
                }
                &#device
            }
        }
    }
}

/// Augment the individual led nodes.  This provides a safe wrapper that can be used to directly use
/// these nodes.
struct GpioLedsAugment;

impl Augment for GpioLedsAugment {
    // GPIO Leds are nodes whose parent is compatible with "gpio-leds".
    fn is_compatible(&self, node: &Node) -> bool {
        if let Some(parent) = node.parent.borrow().as_ref() {
            parent.is_compatible("gpio-leds")
        } else {
            false
        }
    }

    fn generate(&self, node: &Node) -> TokenStream {
        // TODO: Generalize this.
        let words = node.get_words("gpios").unwrap();
        let target = if let Word::Phandle(gpio) = &words[0] {
            gpio.node_ref()
        } else {
            panic!("gpios property in node is empty");
        };

        // At this point, we support gpio-cells of 2.
        if target.get_number("#gpio-cells") != Some(2) {
            panic!("gpios only support with #gpio-cells of 2");
        }

        if words.len() != 3 {
            panic!("gpio-leds, gpios property expected to have only one entry");
        }

        let pin = words[1].get_number().unwrap();
        let flags = words[2].get_number().unwrap();

        let gpio_route = target.route_to_rust();
        quote! {
            pub fn get_instance() -> crate::sys::gpio_dt_spec {
                unsafe {
                    let port = #gpio_route :: get_instance();
                    crate::sys::gpio_dt_spec {
                        port,
                        pin: #pin as crate::sys::gpio_pin_t,
                        dt_flags: #flags as crate::sys::gpio_dt_flags_t,
                    }
                }
            }
        }
    }
}
