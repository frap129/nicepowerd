# nicepowerd
**nicepowerd** is a DE-independent power profile management daemon. The aim of this project is to develop methods for predicting when a user will need a higher perofmance state, and keeping the machine in low power states otherwise. As machines and user needs vary, this daemon will not handle power-related tunables, but will execute programs and scripts provided by the user corresponding to each of the power profiles.

## Goals
- [ ] Manage 3 power states: power, balance, and performance
- [ ] Dynamic scheduling for profile determination
- [ ] Use the nice values of active tasks to predict user need
- [ ] Simple interface for providing user-created power profiles
- [ ] Enforce low-power profile at a set battery percentage
