//! The data file crosses the boundary as a view over the `Arc<DataFile>` the
//! core already shares: `client__get_datafile` retains the `Arc` as the
//! payload's `owner` and every string borrows from it, so nothing but the
//! arrays of views is allocated, and no string bytes are copied.

use std::sync::Arc;

use kameleoon_core::types::{DataFile, FeatureFlag, Rule};

use crate::{
    array_ffi::{FfiArray, FfiKeyValuePair},
    raw_str_ffi::BorrowedStr,
    variation_ffi::{release_variation_map_views, variation_map_view, FfiVariationMap},
    FreeRaw, StructPtr,
};

#[repr(C)]
#[derive(Clone, Copy)]
pub struct FfiDataFile {
    pub feature_flags: FfiArray<FfiKeyValuePair<BorrowedStr, FfiFeatureFlag>>,
    pub date_modified: u64,
    /// The retained `Arc<DataFile>` the views borrow from.
    pub owner: StructPtr,
}

impl FfiDataFile {
    pub fn owned(datafile: Arc<DataFile>) -> Self {
        let feature_flags = datafile
            .feature_flags
            .iter()
            .map(|(key, feature_flag)| FfiKeyValuePair {
                key: key.as_ref().into(),
                value: FfiFeatureFlag::view(feature_flag),
            })
            .collect::<Vec<_>>();

        Self {
            feature_flags: feature_flags.into(),
            date_modified: datafile.date_modified,
            owner: Arc::into_raw(datafile) as StructPtr,
        }
    }
}

impl FreeRaw for FfiDataFile {
    fn free(&self) {
        for pair in self.feature_flags.as_slice() {
            pair.value.release_views();
        }
        self.feature_flags.shallow_free();
        // SAFETY: produced by `Self::owned` from `Arc::into_raw`.
        drop(unsafe { Arc::from_raw(self.owner as *const DataFile) });
    }
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct FfiFeatureFlag {
    pub environment_enabled: bool,
    pub default_variation_key: BorrowedStr,
    pub variations: FfiVariationMap,
    pub rules: FfiArray<FfiRule>,
}

impl FfiFeatureFlag {
    fn view(feature_flag: &FeatureFlag) -> Self {
        let rules = feature_flag.rules.iter().map(FfiRule::view).collect::<Vec<_>>();
        Self {
            environment_enabled: feature_flag.environment_enabled,
            default_variation_key: feature_flag.default_variation_key.as_ref().into(),
            variations: variation_map_view(&feature_flag.variations),
            rules: rules.into(),
        }
    }

    fn release_views(&self) {
        release_variation_map_views(self.variations);
        for rule in self.rules.as_slice() {
            rule.release_views();
        }
        self.rules.shallow_free();
    }
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct FfiRule {
    pub variations: FfiVariationMap,
}

impl FfiRule {
    fn view(rule: &Rule) -> Self {
        Self {
            variations: variation_map_view(&rule.variations),
        }
    }

    fn release_views(&self) {
        release_variation_map_views(self.variations);
    }
}

#[cfg(test)]
mod tests {
    use std::collections::HashMap;

    use std::mem::size_of;

    use kameleoon_core::types::{JsonValue, Variable, Variation};

    use super::*;
    use crate::{test_utils::watch, variable_ffi::FfiVariable, variation_ffi::FfiVariation};

    fn variation(key: &str, value: Arc<str>) -> Variation {
        Variation {
            key: Arc::from(key),
            name: Arc::from("Name"),
            id: Some(1),
            experiment_id: Some(2),
            variables: vec![Variable {
                key: Arc::from("variable"),
                kind: Arc::from("STRING"),
                value: JsonValue::String(value),
            }],
        }
    }

    #[test]
    fn datafile_view_keeps_nested_snapshot_alive_after_replacement() {
        let text: Arc<str> = Arc::from("snapshot value");
        let weak_text = Arc::downgrade(&text);
        let mut current = Arc::new(DataFile {
            date_modified: 42,
            feature_flags: HashMap::from([(
                Arc::from("feature"),
                FeatureFlag {
                    environment_enabled: true,
                    default_variation_key: Arc::from("on"),
                    variations: HashMap::from([(Arc::from("on"), variation("on", text.clone()))]),
                    rules: vec![Rule {
                        variations: HashMap::from([(Arc::from("off"), variation("off", text))]),
                    }],
                },
            )]),
        });
        let weak_snapshot = Arc::downgrade(&current);
        let feature_key_ptr = current.feature_flags.keys().next().unwrap().as_ptr();
        let payload = FfiDataFile::owned(current.clone());
        assert_eq!(Arc::strong_count(&current), 2);

        // Mirrors replacing the cached snapshot while a foreign caller holds
        // a view: every nested string must remain readable until free.
        current = Arc::new(DataFile {
            date_modified: 43,
            feature_flags: HashMap::new(),
        });
        assert_eq!(weak_snapshot.strong_count(), 1);
        assert_eq!(payload.date_modified, 42);
        assert_eq!(payload.feature_flags.len, 1);
        let pair = &payload.feature_flags.as_slice()[0];
        assert_eq!(pair.key.str as *const u8, feature_key_ptr);
        assert_eq!(<&str>::from(pair.key), "feature");
        let flag = &pair.value;
        assert!(flag.environment_enabled);
        assert_eq!(<&str>::from(flag.default_variation_key), "on");
        assert_eq!(flag.rules.len, 1);
        for (map, expected_key) in [(flag.variations, "on"), (flag.rules.as_slice()[0].variations, "off")] {
            assert_eq!(map.len, 1);
            let entry = &map.as_slice()[0];
            assert_eq!(<&str>::from(entry.key), expected_key);
            assert_eq!(<&str>::from(entry.value.key), expected_key);
            assert_eq!(<&str>::from(entry.value.name), "Name");
            assert_eq!(entry.value.id, 1);
            assert_eq!(entry.value.experiment_id, 2);
            assert_eq!(entry.value.variables.as_slice().len(), 1);
        }
        assert_eq!(weak_text.strong_count(), 2);

        payload.free();
        assert!(weak_snapshot.upgrade().is_none(), "free must release the retained snapshot");
        assert!(weak_text.upgrade().is_none(), "free must release nested variable values");
        assert_eq!(current.date_modified, 43);
    }

    #[test]
    fn datafile_free_releases_every_view_array_and_the_snapshot() {
        let text: Arc<str> = Arc::from("value");
        let snapshot = Arc::new(DataFile {
            date_modified: 1,
            feature_flags: HashMap::from([(
                Arc::from("feature"),
                FeatureFlag {
                    environment_enabled: true,
                    default_variation_key: Arc::from("on"),
                    variations: HashMap::from([(Arc::from("on"), variation("on", text.clone()))]),
                    rules: vec![Rule {
                        variations: HashMap::from([(Arc::from("off"), variation("off", text))]),
                    }],
                },
            )]),
        });
        let weak_snapshot = Arc::downgrade(&snapshot);

        let payload = FfiDataFile::owned(snapshot);
        let flag = &payload.feature_flags.as_slice()[0].value;
        let rule = &flag.rules.as_slice()[0];
        // Every array the payload allocated, root to leaf.
        let arrays = [
            (watch(payload.feature_flags.ptr), size_of::<FfiKeyValuePair<BorrowedStr, FfiFeatureFlag>>(), "flags"),
            (watch(flag.variations.ptr), size_of::<FfiKeyValuePair<BorrowedStr, FfiVariation>>(), "flag variations"),
            (watch(flag.rules.ptr), size_of::<FfiRule>(), "rules"),
            (watch(rule.variations.ptr), size_of::<FfiKeyValuePair<BorrowedStr, FfiVariation>>(), "rule variations"),
            (watch(flag.variations.as_slice()[0].value.variables.ptr), size_of::<FfiVariable>(), "flag variables"),
            (watch(rule.variations.as_slice()[0].value.variables.ptr), size_of::<FfiVariable>(), "rule variables"),
        ];
        assert!(arrays.iter().all(|(w, _, _)| w.freed_size().is_none()), "the handover must free nothing");
        assert_eq!(weak_snapshot.strong_count(), 1, "the payload holds the only reference");

        payload.free();

        for (array, expected, what) in &arrays {
            assert_eq!(array.freed_size(), Some(*expected), "free must release the {what} array");
        }
        assert!(weak_snapshot.upgrade().is_none(), "free must release the retained snapshot");
    }

    #[test]
    fn datafile_view_preserves_an_externally_retained_snapshot_when_freed() {
        let source = Arc::new(DataFile {
            date_modified: 7,
            feature_flags: HashMap::from([(
                Arc::from("empty_feature"),
                FeatureFlag {
                    environment_enabled: false,
                    default_variation_key: Arc::from(""),
                    variations: HashMap::new(),
                    rules: vec![Rule {
                        variations: HashMap::new(),
                    }],
                },
            )]),
        });
        let payload = FfiDataFile::owned(source.clone());
        let flag = &payload.feature_flags.as_slice()[0].value;
        assert_eq!(<&str>::from(flag.default_variation_key), "");
        assert!(flag.variations.as_slice().is_empty());
        assert!(flag.rules.as_slice()[0].variations.as_slice().is_empty());

        payload.free();
        assert_eq!(Arc::strong_count(&source), 1);
        assert_eq!(source.feature_flags.len(), 1);
    }

    #[test]
    fn empty_datafile_view_can_be_released() {
        let payload = FfiDataFile::owned(Arc::new(DataFile {
            date_modified: 0,
            feature_flags: HashMap::new(),
        }));
        assert!(payload.feature_flags.as_slice().is_empty());
        payload.free();
    }
}
