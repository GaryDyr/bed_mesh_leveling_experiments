This repository is a supplement to the larger discussion created as a record of transforming a stock Sovol SV06 with the original Marlin software to open source Klipper software, using a BigTreeTech Pad 7 as the secondary mcu. The original conversion file can be found in a separate repository.

This discussion, as a pdf file, expands on the bed leveling process, because of confounding issues that developed while trying to produce uniform first layer prints. No matter what typical procedures were applied: bed shimming, z tramming, X Axis Twist compensation, and bed mesh calibrations, all failed individually or in combination to produce uniform first layer prints.

The discussion describes developing a manual compensation matrix that is added to an inductive probe derived bed mesh to achieve reliable first layer prints, and consistent print behavior for all prints. An accompanying Excel file shows how the data is derived and treated to produce the compensator.

In addition, a DIY touch probe is modeled that can be used as an adjunct to inductive measurements. The probe uses an ESP32 to take z height data.