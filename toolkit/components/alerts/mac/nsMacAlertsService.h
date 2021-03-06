/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef nsGrowlAlertsService_h_
#define nsGrowlAlertsService_h_

#include "nsIAlertsService.h"
#include "nsIObserver.h"

struct GrowlDelegateWrapper;
struct NotificationCenterDelegateWrapper;

class nsMacAlertsService : public nsIAlertsService,
                           public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIALERTSSERVICE
  NS_DECL_NSIOBSERVER

  nsMacAlertsService();
  nsresult Init();

private:
  virtual ~nsMacAlertsService();
  nsresult InitGrowl();
  nsresult InitNotificationCenter();

  GrowlDelegateWrapper* mGrowlDelegate;
  NotificationCenterDelegateWrapper* mNCDelegate;
};

#endif // nsGrowlAlertsService_h_
