# Sensors

## Perspective camera — `type="perspective"`

A standard pinhole camera with an optional thin-lens depth-of-field model. The camera is defined inside a `<sensor>` element together with a `<sampler>` and a `<film>`.

### Parameters

| Name | Type | Default | Description |
|------|------|---------|-------------|
| `fov` | float | `45` | Vertical field of view in degrees. |
| `lookAt` | transform | — | Camera position, target, and up vector (see below). |
| `apertureRadius` or `aperture` | float | `0` | Lens radius. `0` gives a pinhole with infinite depth of field. |
| `focusDistance` or `focalDistance` | float | `1` | Distance from the camera origin to the focal plane. Only has an effect when aperture > 0. |

### Film parameters (inside `<film>`)

| Name | Type | Default | Description |
|------|------|---------|-------------|
| `width` | integer | `640` | Image width in pixels. |
| `height` | integer | `480` | Image height in pixels. |

### Sampler parameters (inside `<sampler>`)

| Name | Type | Default | Description |
|------|------|---------|-------------|
| `sampleCount` | integer | `1` | Samples per pixel. Higher values reduce noise. |

### Camera orientation

The `<lookAt>` element inside `<transform name="toWorld">` specifies the camera pose:

```xml
<transform name="toWorld">
    <lookAt origin="0, 2, 6" target="0, 0, 0" up="0, 1, 0"/>
</transform>
```

- `origin` — world-space position of the camera.
- `target` — point the camera is aimed at.
- `up` — world-space up vector (usually `0, 1, 0`).

### Example — pinhole

```xml
<sensor type="perspective">
    <float name="fov" value="40"/>
    <transform name="toWorld">
        <lookAt origin="0, 2, 5" target="0, 0, 0" up="0, 1, 0"/>
    </transform>
    <sampler type="independent">
        <integer name="sampleCount" value="128"/>
    </sampler>
    <film type="hdrfilm">
        <integer name="width"  value="1280"/>
        <integer name="height" value="720"/>
    </film>
</sensor>
```

### Example — depth of field

Set `apertureRadius` to a positive value and `focusDistance` to the distance of the subject from the camera:

```xml
<sensor type="perspective">
    <float name="fov" value="35"/>
    <float name="apertureRadius" value="0.1"/>
    <float name="focusDistance"  value="6"/>
    <transform name="toWorld">
        <lookAt origin="0, 1.5, 8" target="0, 0.5, 0" up="0, 1, 0"/>
    </transform>
    <sampler type="independent">
        <integer name="sampleCount" value="256"/>
    </sampler>
    <film type="hdrfilm">
        <integer name="width"  value="1280"/>
        <integer name="height" value="720"/>
    </film>
</sensor>
```

Larger `apertureRadius` produces a shallower depth of field (more blur). Objects at exactly `focusDistance` metres from the camera origin are in sharp focus.
