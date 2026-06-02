# Emitters

## Point light — `type="point"`

An infinitesimally small light source that radiates equally in all directions. Irradiance falls off as 1/r². Point lights are defined as top-level `<emitter>` elements.

### Parameters

| Name | Type | Default | Description |
|------|------|---------|-------------|
| `position` | point | `0, 0, 0` | World-space position of the light. |
| `intensity` | rgb | `1, 1, 1` | Emitted power per steradian. High values are normal; the 1/r² falloff makes the perceived brightness scene-scale dependent. |

### Example

```xml
<emitter type="point">
    <point name="position" x="0" y="3" z="0"/>
    <rgb name="intensity" value="100, 80, 60"/>
</emitter>
```

Multiple point lights may be added to the same scene.

---

## Area emitter — `type="area"`

An emissive surface that contributes light in the hemisphere above its geometric normal. Any shape can be turned into an area light by nesting an `<emitter type="area">` inside its `<shape>` block. In the path integrator, area lights are sampled using MIS (see [Integrators](integrators.md)).

### Parameters

| Name | Type | Default | Description |
|------|------|---------|-------------|
| `radiance` | rgb | `1, 1, 1` | Emitted radiance (per unit area, per steradian). |

### Example — rectangle ceiling light

```xml
<shape type="rectangle">
    <transform name="toWorld">
        <scale x="1" y="1" z="1"/>
        <translate x="0" y="2.5" z="0"/>
        <rotate angle="90" x="1" y="0" z="0"/>
    </transform>
    <emitter type="area">
        <rgb name="radiance" value="8, 6, 4"/>
    </emitter>
</shape>
```

### Example — emissive sphere

```xml
<shape type="sphere">
    <point name="center" x="0" y="2" z="0"/>
    <float name="radius" value="0.25"/>
    <emitter type="area">
        <rgb name="radiance" value="20, 20, 20"/>
    </emitter>
</shape>
```

Named emissive materials may also be declared at the top level using `<bsdf type="emitter">` and then referenced from a shape with `<ref>`.

---

## Environment map — `type="envmap"`

An equirectangular HDR image wrapped around the scene as an infinite distant light source. The map is importance-sampled based on luminance, giving proportionally more samples to bright regions. Defined as a top-level `<emitter>` element.

### Parameters

| Name | Type | Default | Description |
|------|------|---------|-------------|
| `filename` | string | *(required)* | Path to the environment map image, relative to the scene file. Supports `.exr` (linear HDR) and `.hdr` (Radiance HDR). LDR images (`.png`, `.jpg`) are accepted but will lack dynamic range. |

### Example

```xml
<emitter type="envmap">
    <string name="filename" value="textures/sky.exr"/>
</emitter>
```

When an environment map is present, rays that escape the scene are coloured by the map rather than the default sky colour. In the path integrator the environment map is sampled via MIS alongside BSDF sampling.

Only one environment map per scene is supported. A point or area light can coexist with an environment map.
