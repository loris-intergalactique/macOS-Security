/*
 * Copyright (c) 2016 Apple Inc. All Rights Reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 * @APPLE_LICENSE_HEADER_END@
 */

#import <Foundation/Foundation.h>

#if OCTAGON

#import "keychain/ckks/CKKS.h"
#import "keychain/ckks/CKKSGroupOperation.h"

NS_ASSUME_NONNULL_BEGIN

@class CKKSKeychainView;

// CKKS's state machine depends on its operations performing callbacks to move the state, but this means
// the operations cannot be reused outside of the state machine context.
// Therefore, split this operation into two: one which does the work, and another which does the CKKS state manipulation.

@interface CKKSProcessReceivedKeysOperation : CKKSResultOperation
@property (weak) CKKSKeychainView* ckks;

@property CKKSZoneKeyState* nextState;

- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithCKKSKeychainView:(CKKSKeychainView*)ckks;
@end

@interface CKKSProcessReceivedKeysStateMachineOperation : CKKSResultOperation
@property (weak) CKKSKeychainView* ckks;

- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithCKKSKeychainView:(CKKSKeychainView*)ckks;
@end

NS_ASSUME_NONNULL_END
#endif // OCTAGON
