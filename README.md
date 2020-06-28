[![paypal](https://www.paypalobjects.com/en_US/i/btn/btn_donate_SM.gif)](https://www.paypal.com/paypalme/rbtylee)

# forecast2.0

This is a module for moksha or e17 to display weather information on your desktop or shelf. It is a refactoring of the old e17 [forecast module](https://github.com/JeffHoogland/moksha-modules-extra/tree/master/forecasts) . That module is sadly dead being killed by yahoo API changes:

>As of Thursday, Jan. 3, 2019, the weather.yahooapis.com and query.yahooapis.com for Yahoo Weather API will be retired.

The Bodhi Team was never comfortable with the new API Terms of service and abondoned the module with the goal of finding a more suitable weather API. To that end [Štefan 'the waiter' Uram](https://github.com/thewaiter) settled on using [wttr.in](http://wttr.in/) and began work on porting the old forecast code over to this new API. [Robert (ylee) Wiley](https://github.com/rbtylee) naturally helped some in this project. Special thanks to [Igor Chubin](https://github.com/chubin) for consultation and assisitance in this endeavor.

# Dependencies

* The usual build tools, autopoint libtool intltool pkg-config  autotools-dev
* [EFL](https://www.enlightenment.org/download)
* [Moksha](https://github.com/JeffHoogland/moksha)

# Installation

It is recommended Bodhi 5.0 users install from Bodhi's repo:

```ShellSession
sudo apt update
sudo apt install moksha-module-forecast
```

Other users need to compile the code:

First install all the needed dependencies. Note this includes not only EFL but the EFL header files. If you have compiled and installed EFL, and Moksha from source code this should be no problem. 

Then the usual:

```ShellSession
./autogen.sh
make
sudo make install
```

# Pure enlightenment

It is our hope to create a branch for e17. Then to install in e17 clone the repo and switch to the e17 branch and compile as usual.

# Reporting bugs

Please use the GitHub issue tracker for any bugs or feature suggestions.

# Contributing

Help is always Welcome, as with all Open Source Projects the more people that help the better it gets!

Please submit patches to the code or documentation as GitHub pull requests!

Contributions must be licensed under this project's copyright (see COPYING).

# Help wanted

Support for only a few languages are currently provided. The needed PO files have been created but we are requesting users of this modules contribute the needed missing localizations.

Developers may wish to examine our todo file and help implement future features.

Thanks in advance.

# Support This Project

This module is part of our current project to restore to functioning all broken e17 modues we know about. These modules can be broken by enlightenment code changes or EFL API changes or in this case other API changes. 

Donations to [Bodhi Linux](https://www.bodhilinux.com/donate/) would be greatly appreciated and keep our distro moving along. But if you like the work we do for Bodhi and wish to see more of it, we'd be happy about a donation. You can donate via [PayPall](https://www.paypal.com/paypalme/rbtylee). If you mention this module, we will forward the donation to Štefan 'the waiter' Uram as he did most of the initial developement work involved.

# License

This software is released under the same License used in alot of the other Enlightenment projects. It is a custom license but fully Open Source. Please see the included [COPYING](https://github.com/rbtylee/launcher-spellchecker/blob/master/COPYING) file and for a less legalese explanation [COPYING-PLAIN](https://github.com/rbtylee/launcher-spellchecker/blob/master/COPYING-PLAIN).

Simply put, this software is free to use, modify and redistribute as you see fit. I do ask that you keep the copyright notice the same in any modifications.

The debian files are  released the terms of the [GNU General Public License](https://www.gnu.org/licenses/gpl.html) as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.


# Credits

Reusing code from the original forecast module:

Rewritten again by:
* _*Viktor 'urandom' Kojouharov*_

Bodhi specific modifications, wttr.in API changes, improvements and code modernization go to :
* _*Robert Wiley*_
* _*Štefan Uram*_

A special thanks to Igor Chubin for his wonderful {wttr.in] (https://github.com/chubin/wttr.in) command line tool and website.

