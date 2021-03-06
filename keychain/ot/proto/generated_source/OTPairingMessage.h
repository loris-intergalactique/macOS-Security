// This file was automatically generated by protocompiler
// DO NOT EDIT!
// Compiled from OTPairingMessage.proto

#import <Foundation/Foundation.h>
#import <ProtocolBuffer/PBCodable.h>

@class OTSponsorToApplicantRound1M2;
@class OTApplicantToSponsorRound2M1;
@class OTSponsorToApplicantRound2M2;

#ifdef __cplusplus
#define OTPAIRINGMESSAGE_FUNCTION extern "C" __attribute__((visibility("hidden")))
#else
#define OTPAIRINGMESSAGE_FUNCTION extern __attribute__((visibility("hidden")))
#endif

/**
 * Claimed for a field, but never used
 * reserved 3;
 */
__attribute__((visibility("hidden")))
@interface OTPairingMessage : PBCodable <NSCopying>
{
    OTSponsorToApplicantRound1M2 *_epoch;
    OTApplicantToSponsorRound2M1 *_prepare;
    OTSponsorToApplicantRound2M2 *_voucher;
}


@property (nonatomic, readonly) BOOL hasEpoch;
@property (nonatomic, retain) OTSponsorToApplicantRound1M2 *epoch;

@property (nonatomic, readonly) BOOL hasPrepare;
@property (nonatomic, retain) OTApplicantToSponsorRound2M1 *prepare;

@property (nonatomic, readonly) BOOL hasVoucher;
@property (nonatomic, retain) OTSponsorToApplicantRound2M2 *voucher;

// Performs a shallow copy into other
- (void)copyTo:(OTPairingMessage *)other;

// Performs a deep merge from other into self
// If set in other, singular values in self are replaced in self
// Singular composite values are recursively merged
// Repeated values from other are appended to repeated values in self
- (void)mergeFrom:(OTPairingMessage *)other;

OTPAIRINGMESSAGE_FUNCTION BOOL OTPairingMessageReadFrom(__unsafe_unretained OTPairingMessage *self, __unsafe_unretained PBDataReader *reader);

@end

