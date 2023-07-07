/**
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "WarpSelectImpl.cuh"

namespace faiss {
namespace gpu {

WARP_SELECT_IMPL(float, true, 1, 1);
WARP_SELECT_IMPL(float, false, 1, 1);

} // namespace gpu
} // namespace faiss
