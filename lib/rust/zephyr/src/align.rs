//! Alignment
//!
//! Natively, the align attribute in rust does not allow anything other than an integer literal.
//! However, Zephyr will define the external alignment based on numeric constants.  This defines a
//! bit of a trick to enforce alignment of structs to values by defined constants.
//!
//! Thanks to Chayim Refael Friedman for help with this.

pub struct AlignAsStruct;

pub trait AlignAsTrait<const N: usize> {
    type Aligned;
}

macro_rules! impl_alignas {
    ( $($align:literal),* $(,)? ) => {
        $(
            const _: () = {
                #[repr(align($align))]
                pub struct Aligned;
                impl AlignAsTrait<$align> for AlignAsStruct {
                    type Aligned = Aligned;
                }
            };
        )*
    };
}
// This can be expanded as needed.
impl_alignas!(1, 2, 4, 8, 16, 32, 64, 128, 256);

/// Align a given struct to a given alignment.  To use this, just include `AlignAs<N>` as the first
/// member of the struct.
#[repr(transparent)]
pub struct AlignAs<const N: usize>([<AlignAsStruct as AlignAsTrait<N>>::Aligned; 0])
    where
    AlignAsStruct: AlignAsTrait<N>;
