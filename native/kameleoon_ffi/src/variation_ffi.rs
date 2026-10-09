//! Variations cross the boundary as *views*: every string borrows from the
//! `Arc<str>` data of a core `Variation` that stays alive until the payload is
//! released, so no string bytes are copied on the Rust side. Only the arrays
//! of views are allocated (one `Vec` per array), plus standalone boxed owners.
//! The root payload's `free` reclaims its arrays and releases its owner.
//!
//! Every root result is a view plus an `owner` that keeps the core value
//! alive: a boxed `Variation` for `client__get_variation`, a boxed map for
//! `client__get_variations`, the `Arc<DataFile>` for `client__get_datafile`.
//! Views themselves never own anything.

use std::{collections::HashMap, sync::Arc};

use kameleoon_core::types::Variation;

use crate::{
    array_ffi::{FfiArray, FfiKeyValuePair},
    raw_str_ffi::BorrowedStr,
    variable_ffi::FfiVariable,
    FreeRaw, StructPtr,
};

/// View of a variation; `id`/`experiment_id` are `u32::MAX` when absent.
#[repr(C)]
#[derive(Clone, Copy)]
pub struct FfiVariation {
    pub key: BorrowedStr,
    pub name: BorrowedStr,
    pub id: u32,
    pub experiment_id: u32,
    pub variables: FfiArray<FfiVariable>,
}

impl FfiVariation {
    pub(crate) fn view(var: &Variation) -> Self {
        let variables: Vec<FfiVariable> = var.variables.iter().map(FfiVariable::from).collect();
        FfiVariation {
            key: var.key.as_ref().into(),
            name: var.name.as_ref().into(),
            id: var.id.unwrap_or(u32::MAX),
            experiment_id: var.experiment_id.unwrap_or(u32::MAX),
            variables: variables.into(),
        }
    }

    /// Releases the view arrays (not the variation itself).
    pub(crate) fn release_views(&self) {
        self.variables.shallow_free();
    }
}

/// Result of `client__get_variation`.
#[repr(C)]
#[derive(Clone, Copy)]
pub struct FfiOwnedVariation {
    pub variation: FfiVariation,
    /// Boxed core `Variation` the view borrows from.
    pub owner: StructPtr,
}

impl FfiOwnedVariation {
    pub fn owned(var: Variation) -> Self {
        let var = Box::new(var);
        let variation = FfiVariation::view(&var);
        FfiOwnedVariation {
            variation,
            owner: Box::into_raw(var) as StructPtr,
        }
    }
}

impl FreeRaw for FfiOwnedVariation {
    fn free(&self) {
        self.variation.release_views();
        // SAFETY: produced by `Self::owned` from `Box::into_raw`.
        drop(unsafe { Box::from_raw(self.owner as *mut Variation) });
    }
}

/// Views of a `HashMap<Arc<str>, Variation>`, keyed by feature key.
pub type FfiVariationMap = FfiArray<FfiKeyValuePair<BorrowedStr, FfiVariation>>;

pub(crate) fn variation_map_view(map: &HashMap<Arc<str>, Variation>) -> FfiVariationMap {
    map.iter()
        .map(|(key, var)| FfiKeyValuePair {
            key: key.as_ref().into(),
            value: FfiVariation::view(var),
        })
        .collect::<Vec<_>>()
        .into()
}

pub(crate) fn release_variation_map_views(map: FfiVariationMap) {
    for pair in map.as_slice() {
        pair.value.release_views();
    }
    map.shallow_free();
}

/// Result of `client__get_variations`.
#[repr(C)]
#[derive(Clone, Copy)]
pub struct FfiVariations {
    pub variations: FfiVariationMap,
    /// Boxed `HashMap<Arc<str>, Variation>` the views borrow from.
    pub owner: StructPtr,
}

impl FfiVariations {
    pub fn owned(map: HashMap<Arc<str>, Variation>) -> Self {
        let map = Box::new(map);
        let variations = variation_map_view(&map);
        FfiVariations {
            variations,
            owner: Box::into_raw(map) as StructPtr,
        }
    }
}

impl FreeRaw for FfiVariations {
    fn free(&self) {
        release_variation_map_views(self.variations);
        // SAFETY: produced by `Self::owned` from `Box::into_raw`.
        drop(unsafe { Box::from_raw(self.owner as *mut HashMap<Arc<str>, Variation>) });
    }
}

#[cfg(test)]
mod tests {
    use std::mem::size_of;

    use kameleoon_core::types::{JsonValue, Variable};

    use super::*;
    use crate::test_utils::watch;

    fn variation(key: &str) -> Variation {
        Variation {
            key: Arc::from(key),
            name: Arc::from("Name"),
            id: Some(1),
            experiment_id: None,
            variables: Vec::new(),
        }
    }

    #[test]
    fn owned_variation_borrows_from_its_owner_and_frees_everything() {
        let mut var = variation("on");
        var.variables.push(Variable {
            key: Arc::from("variable"),
            kind: Arc::from("STRING"),
            value: JsonValue::String(Arc::from("value")),
        });
        let key_ptr = var.key.as_ptr();
        let weak_key = Arc::downgrade(&var.key);

        let payload = FfiOwnedVariation::owned(var);
        let owner = watch(payload.owner);
        let variables = watch(payload.variation.variables.ptr);

        let view = payload.variation;
        assert_eq!(view.key.str as *const u8, key_ptr, "view must point into the Arc<str> data");
        assert_eq!(<&str>::from(view.key), "on");
        assert_eq!(view.id, 1);
        assert_eq!(view.experiment_id, u32::MAX);
        assert_eq!(view.variables.len, 1);
        assert!(!payload.owner.is_null());

        payload.free();

        assert_eq!(owner.freed_size(), Some(size_of::<Variation>()), "free must drop the boxed owner");
        assert_eq!(variables.freed_size(), Some(size_of::<FfiVariable>()), "free must release the view array");
        assert!(weak_key.upgrade().is_none(), "dropping the owner must release its strings");
    }

    #[test]
    fn owned_variation_retains_nested_values_until_freed() {
        let text: Arc<str> = Arc::from("retained variable value");
        let weak_text = Arc::downgrade(&text);
        let mut var = variation("on");
        let weak_key = Arc::downgrade(&var.key);
        var.variables.push(Variable {
            key: Arc::from("variable"),
            kind: Arc::from("STRING"),
            value: JsonValue::String(text),
        });

        let payload = FfiOwnedVariation::owned(var);
        assert_eq!(payload.variation.variables.as_slice().len(), 1);
        assert_eq!(weak_key.strong_count(), 1);
        assert_eq!(weak_text.strong_count(), 1);

        payload.free();
        assert!(weak_key.upgrade().is_none());
        assert!(weak_text.upgrade().is_none());
    }

    #[test]
    fn variations_view_every_entry() {
        let mut map = HashMap::new();
        map.insert(Arc::from("ff_a"), variation("on"));
        map.insert(Arc::from("ff_b"), variation("off"));

        let payload = FfiVariations::owned(map);

        assert_eq!(payload.variations.len, 2);
        assert!(!payload.owner.is_null());
        payload.free();
    }

    #[test]
    fn variations_owner_retains_map_keys_and_nested_variable_values() {
        let feature_key: Arc<str> = Arc::from("feature");
        let weak_feature_key = Arc::downgrade(&feature_key);
        let text: Arc<str> = Arc::from("nested value");
        let weak_text = Arc::downgrade(&text);
        let mut var = variation("on");
        var.variables.push(Variable {
            key: Arc::from("variable"),
            kind: Arc::from("JSON"),
            value: JsonValue::Json(text),
        });
        let payload = FfiVariations::owned(HashMap::from([(feature_key, var)]));
        let pair = &payload.variations.as_slice()[0];

        assert_eq!(<&str>::from(pair.key), "feature");
        assert_eq!(<&str>::from(pair.value.key), "on");
        assert_eq!(pair.value.variables.as_slice().len(), 1);
        assert_eq!(weak_feature_key.strong_count(), 1);
        assert_eq!(weak_text.strong_count(), 1);

        payload.free();
        assert!(weak_feature_key.upgrade().is_none());
        assert!(weak_text.upgrade().is_none());
    }

    #[test]
    fn variations_free_releases_owner_and_every_view_array() {
        let mut on = variation("on");
        on.variables.push(Variable {
            key: Arc::from("a"),
            kind: Arc::from("STRING"),
            value: JsonValue::String(Arc::from("value")),
        });
        let mut off = variation("off");
        off.variables.push(Variable {
            key: Arc::from("b"),
            kind: Arc::from("NUMBER"),
            value: JsonValue::Number(1.0),
        });
        let feature_key: Arc<str> = Arc::from("feature");
        let weak_feature_key = Arc::downgrade(&feature_key);
        let map = HashMap::from([(feature_key, on), (Arc::from("other"), off)]);

        let payload = FfiVariations::owned(map);
        let owner = watch(payload.owner);
        let entries = watch(payload.variations.ptr);
        let variables: Vec<_> =
            payload.variations.as_slice().iter().map(|pair| watch(pair.value.variables.ptr)).collect();
        assert_eq!(payload.variations.len, 2);
        assert!(variables.iter().all(|w| w.freed_size().is_none()), "the handover must free nothing");

        payload.free();

        assert_eq!(owner.freed_size(), Some(size_of::<HashMap<Arc<str>, Variation>>()), "free must drop the boxed map");
        assert_eq!(
            entries.freed_size(),
            Some(2 * size_of::<FfiKeyValuePair<BorrowedStr, FfiVariation>>()),
            "free must release the entry array"
        );
        for variables in &variables {
            assert_eq!(variables.freed_size(), Some(size_of::<FfiVariable>()), "free must release variables arrays");
        }
        assert!(weak_feature_key.upgrade().is_none(), "dropping the map must release its keys");
    }

    #[test]
    fn empty_variations_payload_can_be_released() {
        let payload = FfiVariations::owned(HashMap::new());
        assert!(payload.variations.as_slice().is_empty());
        payload.free();
    }
}
