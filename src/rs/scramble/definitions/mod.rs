use std::sync::OnceLock;

use cubing::{
    kpuzzle::{KPuzzle, KPuzzleDefinition},
    puzzles::cube2x2x2_kpuzzle,
};

use crate::_internal::{PackedKPattern, PackedKPuzzle};

static CUBE2X2X2_KPUZZLE_CELL: OnceLock<PackedKPuzzle> = OnceLock::new();
// TODO: avoid re-parsing every time
pub(crate) fn cube2x2x2_packed_kpuzzle() -> PackedKPuzzle {
    CUBE2X2X2_KPUZZLE_CELL
        .get_or_init(|| {
            let kpuzzle = cube2x2x2_kpuzzle();
            PackedKPuzzle::try_from(kpuzzle).unwrap()
        })
        .clone()
}

static CUBE3X3X3_CENTERLESS_KPUZZLE_CELL: OnceLock<PackedKPuzzle> = OnceLock::new();
// TODO: avoid re-parsing every time
pub(crate) fn cube3x3x3_centerless_packed_kpuzzle() -> PackedKPuzzle {
    CUBE3X3X3_CENTERLESS_KPUZZLE_CELL
        .get_or_init(|| {
            let json_bytes = include_bytes!("3x3x3-centerless.kpuzzle.json");
            let def: KPuzzleDefinition = serde_json::from_slice(json_bytes).unwrap();
            let kpuzzle: KPuzzle = def.try_into().unwrap();
            PackedKPuzzle::try_from(kpuzzle).unwrap()
        })
        .clone()
}

static CUBE3X3X3_G1_CENTERLESS_PATTERN_CELL: OnceLock<PackedKPattern> = OnceLock::new();
pub(crate) fn cube3x3x3_g1_target_pattern() -> PackedKPattern {
    CUBE3X3X3_G1_CENTERLESS_PATTERN_CELL
        .get_or_init(|| {
            let packed_kpuzzle = cube3x3x3_centerless_packed_kpuzzle();
            PackedKPattern::try_from_json(
                &packed_kpuzzle,
                include_bytes!("3x3x3-G1-centerless.target-pattern.json"),
            )
            .unwrap()
        })
        .clone()
}

static TETRAMINX_KPUZZLE_CELL: OnceLock<PackedKPuzzle> = OnceLock::new();
pub(crate) fn tetraminx_packed_kpuzzle() -> PackedKPuzzle {
    TETRAMINX_KPUZZLE_CELL
        .get_or_init(|| {
            let json_bytes = include_bytes!("tetraminx.kpuzzle.json");
            let def: KPuzzleDefinition = serde_json::from_slice(json_bytes).unwrap();
            let kpuzzle: KPuzzle = def.try_into().unwrap();
            PackedKPuzzle::try_from(kpuzzle).unwrap()
        })
        .clone()
}
