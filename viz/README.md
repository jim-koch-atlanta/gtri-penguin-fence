# Visualization

`plot_mission.py` renders the two-panel figure (`docs/figure.png`) from a
mission GeoJSON: **Panel A** re-projects everything into the mission's local
AEQD meters frame (the honest view), **Panel B** plots the same data as raw
lon/lat (the pole smear that motivates the whole approach).

## Setup (once)

On this project's Debian/Ubuntu environment (no `pip`/`venv`), install the
system packages:

```bash
sudo apt-get install -y python3-pyproj python3-matplotlib
```

Portable alternative (any platform with pip):

```bash
python3 -m venv viz/.venv && viz/.venv/bin/pip install -r viz/requirements.txt
```

## Regenerate

```bash
python3 viz/plot_mission.py [mission.geojson] [figure.png]
```

Defaults: reads `build/mission.geojson`, writes `docs/figure.png`. The input
GeoJSON must carry the top-level `aeqd_center` property (the projection center).
