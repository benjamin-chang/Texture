//
//  ASLog.h
//  Texture
//
//  Copyright (c) 2014-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the /ASDK-Licenses directory of this source tree. An additional
//  grant of patent rights can be found in the PATENTS file in the same directory.
//
//  Modifications to this file made after 4/13/2017 are: Copyright (c) 2017-present,
//  Pinterest, Inc.  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//

#import <AsyncDisplayKit/ASAvailability.h>
#import <Foundation/Foundation.h>
#import <os/log.h>
#import <os/activity.h>

/// The signposts we use. Signposts are grouped by color. The SystemTrace.tracetemplate file
/// should be kept up-to-date with these values.
typedef NS_ENUM(uint32_t, ASSignpostName) {
  // Collection/Table (Blue)
  ASSignpostDataControllerBatch = 300,		// Alloc/layout nodes before collection update.
  ASSignpostRangeControllerUpdate,				// Ranges update pass.
  ASSignpostCollectionUpdate,							// Entire update process, from -endUpdates to [super perform…]

  // Rendering (Green)
  ASSignpostLayerDisplay = 325,						// Client display callout.
  ASSignpostRunLoopQueueBatch,						// One batch of ASRunLoopQueue.

  // Layout (Purple)
  ASSignpostCalculateLayout = 350,				// Start of calculateLayoutThatFits to end. Max 1 per thread.

  // Misc (Orange)
  ASSignpostDeallocQueueDrain = 375,			// One chunk of dealloc queue work. arg0 is count.
  ASSignpostCATransactionLayout,					// The CA transaction commit layout phase.
  ASSignpostCATransactionCommit						// The CA transaction commit post-layout phase.
};

typedef NS_ENUM(uintptr_t, ASSignpostColor) {
  ASSignpostColorBlue,
  ASSignpostColorGreen,
  ASSignpostColorPurple,
  ASSignpostColorOrange,
  ASSignpostColorRed,
  ASSignpostColorDefault
};

static inline ASSignpostColor ASSignpostGetColor(ASSignpostName name, ASSignpostColor colorPref) {
  if (colorPref == ASSignpostColorDefault) {
    return (ASSignpostColor)((name / 25) % 4);
  } else {
    return colorPref;
  }
}

/**
 * The activity tracing system changed a lot between iOS 9 and 10.
 * In iOS 10, the system was merged with logging and became much more powerful
 * and adopted a new API.
 *
 * The legacy API is visible, but its functionality is limited.
 * For example, activities described by os_activity_start/end are not
 * reflected in the log whereas activities described by the newer
 * os_activity_scope are. So unfortunately we must use these iOS 10
 * APIs to get meaningful logging data.
 */
#if OS_LOG_TARGET_HAS_10_12_FEATURES

#define OS_ACTIVITY_NULLABLE nullable
#define as_activity_scope(activity) os_activity_scope(activity)
#define as_activity_apply(activity, block) os_activity_apply(activity, block)
#define as_activity_create(description, parent_activity, flags) os_activity_create(description, parent_activity, flags)

// Log the current backtrace. Note: the backtrace will be leaked. Only call this when debugging or in case of failure.
#define as_log_backtrace(type, log) os_log_with_type(log, type, "backtrace: %p", CFBridgingRetain(NSThread.callStackSymbols));

#else

#define OS_ACTIVITY_NULLABLE
#define as_activity_scope(activity)
#define as_activity_apply(activity, block)
#define as_activity_create(description, parent_activity, flags) (os_activity_t)0
#define as_log_backtrace(type, log)

#endif

#define as_log_create(subsystem, category) AS_AT_LEAST_IOS9 ? os_log_create(subsystem, category) : (os_log_t)0
#define as_log_debug(log, format, ...) AS_AT_LEAST_IOS9 ? os_log_debug(log, format, ##__VA_ARGS__) : (void)0
#define as_log_info(log, format, ...) AS_AT_LEAST_IOS9 ? os_log_info(log, format, ##__VA_ARGS__) : (void)0
#define as_log_error(log, format, ...) AS_AT_LEAST_IOS9 ? os_log_error(log, format, ##__VA_ARGS__) : (void)0
#define as_log_fault(log, format, ...) AS_AT_LEAST_IOS9 ? os_log_fault(log, format, ##__VA_ARGS__) : (void)0
static os_log_t ASLayoutLog;
static os_log_t ASRenderLog;
static os_log_t ASCollectionLog;
#define ASMultiplexImageNodeLogDebug(...)
#define ASMultiplexImageNodeCLogDebug(...)

#define ASMultiplexImageNodeLogError(...)
#define ASMultiplexImageNodeCLogError(...)

#define AS_KDEBUG_ENABLE defined(PROFILE) && __has_include(<sys/kdebug_signpost.h>)

#if AS_KDEBUG_ENABLE

#import <sys/kdebug_signpost.h>

// These definitions are required to build the backward-compatible kdebug trace
// on the iOS 10 SDK.  The kdebug_trace function crashes if run on iOS 9 and earlier.
// It's valuable to support trace signposts on iOS 9, because A5 devices don't support iOS 10.
#ifndef DBG_MACH_CHUD
#define DBG_MACH_CHUD 0x0A
#define DBG_FUNC_NONE 0
#define DBG_FUNC_START 1
#define DBG_FUNC_END 2
#define DBG_APPS 33
#define SYS_kdebug_trace 180
#define KDBG_CODE(Class, SubClass, code) (((Class & 0xff) << 24) | ((SubClass & 0xff) << 16) | ((code & 0x3fff)  << 2))
#define APPSDBG_CODE(SubClass,code) KDBG_CODE(DBG_APPS, SubClass, code)
#endif

// Currently we'll reserve arg3.
#define ASSignpost(name, identifier, arg2, color) \
  AS_AT_LEAST_IOS10 ? kdebug_signpost(name, (uintptr_t)identifier, (uintptr_t)arg2, 0, ASSignpostGetColor(name, color)) \
                    : syscall(SYS_kdebug_trace, APPSDBG_CODE(DBG_MACH_CHUD, name) | DBG_FUNC_NONE, (uintptr_t)identifier, (uintptr_t)arg2, 0, ASSignpostGetColor(name, color));

#define ASSignpostStartCustom(name, identifier, arg2) \
  AS_AT_LEAST_IOS10 ? kdebug_signpost_start(name, (uintptr_t)identifier, (uintptr_t)arg2, 0, 0) \
                    : syscall(SYS_kdebug_trace, APPSDBG_CODE(DBG_MACH_CHUD, name) | DBG_FUNC_START, (uintptr_t)identifier, (uintptr_t)arg2, 0, 0);
#define ASSignpostStart(name) ASSignpostStartCustom(name, self, 0)

#define ASSignpostEndCustom(name, identifier, arg2, color) \
  AS_AT_LEAST_IOS10 ? kdebug_signpost_end(name, (uintptr_t)identifier, (uintptr_t)arg2, 0, ASSignpostGetColor(name, color)) \
                    : syscall(SYS_kdebug_trace, APPSDBG_CODE(DBG_MACH_CHUD, name) | DBG_FUNC_END, (uintptr_t)identifier, (uintptr_t)arg2, 0, ASSignpostGetColor(name, color));
#define ASSignpostEnd(name) ASSignpostEndCustom(name, self, 0, ASSignpostColorDefault)

#else

#define ASSignpost(name, identifier, arg2, color)
#define ASSignpostStartCustom(name, identifier, arg2)
#define ASSignpostStart(name)
#define ASSignpostEndCustom(name, identifier, arg2, color)
#define ASSignpostEnd(name)

#endif
