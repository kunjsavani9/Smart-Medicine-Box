# 3D Models — AUSHADH Enclosure

3D-printable models and editable CAD source for the AUSHADH Smart Medicine Box enclosure.

## Files

| File | Type | Description |
|------|------|-------------|
| `Base.STL` / `Base.SLDPRT` | Print + Source | Main body — houses the electronics, with the keypad cutout (top-left), LCD window (top-right), two buzzer holes (middle-left), and two columns of 7 pill compartments |
| `Cap.STL` / `Cap.SLDPRT` | Print + Source | Compartment lid / cap |
| `Cover.STL` / `Cover.SLDPRT` | Print + Source | Top cover |
| `Assem1.SLDASM` | Assembly | Full SolidWorks assembly of all parts |

- **`.STL`** files are ready to slice and print.
- **`.SLDPRT` / `.SLDASM`** are the editable SolidWorks source files.

## Printing

- **Material:** PLA or PETG
- **Layer height:** 0.2 mm
- **Infill:** 15–20%
- **Supports:** as needed for compartment overhangs and cutouts
- Print `Base`, `Cap`, and `Cover` separately, then assemble.

## Editing in SolidWorks

Open the `.SLDPRT` / `.SLDASM` files directly. When **importing an STL** into SolidWorks, set the units to **millimeters** (*Options → Import → Solid Body*) so dimensions come in correctly.

## Layout

The enclosure follows the project's hand-drawn layout:

- **Keypad** — top-left
- **LCD** — top-right
- **Buzzer** — two holes, middle-left
- **Pill compartments** — two columns of 7 (14 total), covering a full week of twice-daily doses
