# nicepowerd
**nicepowerd** is a DE-independent power profile management daemon. [power-profiles-daemon](https://gitlab.freedesktop.org/hadess/power-profiles-daemon/-/tree/main) is slowly becoming the default power management utility in the Linux community, though it is only integrated into DE's like Gnome and KDE. The plan for `nicepowerd` is to create a service that decides what power profile should be used in place of a DE, and allow the user to customize what happens when any given profile is selected.

The main aim of this project is to develop methods for predicting when a user will need a higher perofmance state, and keeping the machine in low power states otherwise. As machines and user needs vary, this daemon will not handle power-related tunables, but will execute programs and scripts provided by the user corresponding to each of the power profiles.

This is mainly for personal use. Development will be slow, and profile scheduling logic may change. Use with caution.
## Goals
- [x] Manage 3 power states: power, balance, and performance
- [ ] Dynamic scheduling for profile determination
- [ ] Use the nice values of active tasks to predict user need
- [x] Simple interface for providing user-created power profiles
- [x] Enforce low-power profile at a set battery percentage
