# Scene Format

Kestrel reads a subset of the Mitsuba 0.6 XML scene format. A scene file is a UTF-8 XML document whose root element is `<scene>`.

```xml
<?xml version="1.0" encoding="utf-8"?>
<scene version="0.6.0">
    <!-- child elements -->
</scene>
```

## Top-level elements

| Element | Purpose |
|---------|---------|
| `<sensor>` | Camera and film definition. Exactly one per scene. |
| `<bsdf>` | Named material definition, reusable across shapes. |
| `<emitter>` | Scene-wide light source (point or environment map). |
| `<shape>` | Geometry primitive or mesh, with an attached material. |

Elements are parsed in the order they appear. Named BSDFs (`id="..."`) must be declared before any `<shape>` that references them.

## Minimal scene

```xml
<?xml version="1.0" encoding="utf-8"?>
<scene version="0.6.0">

    <sensor type="perspective">
        <float name="fov" value="45"/>
        <transform name="toWorld">
            <lookAt origin="0, 1, 5" target="0, 0, 0" up="0, 1, 0"/>
        </transform>
        <sampler type="independent">
            <integer name="sampleCount" value="64"/>
        </sampler>
        <film type="hdrfilm">
            <integer name="width"  value="640"/>
            <integer name="height" value="480"/>
        </film>
    </sensor>

    <bsdf type="diffuse" id="red">
        <rgb name="reflectance" value="0.8, 0.1, 0.1"/>
    </bsdf>

    <emitter type="point">
        <point name="position" x="0" y="3" z="0"/>
        <rgb name="intensity" value="100, 100, 100"/>
    </emitter>

    <shape type="sphere">
        <point name="center" x="0" y="0" z="0"/>
        <float name="radius" value="1"/>
        <ref id="red"/>
    </shape>

</scene>
```

## Named BSDFs and references

Define a material once at the top level with an `id`, then reference it from any number of shapes:

```xml
<bsdf type="diffuse" id="floor_mat">
    <rgb name="reflectance" value="0.6, 0.6, 0.6"/>
</bsdf>

<shape type="rectangle">
    <ref id="floor_mat"/>
</shape>

<shape type="sphere">
    <float name="radius" value="0.5"/>
    <ref id="floor_mat"/>
</shape>
```

Inline BSDFs (declared inside `<shape>`) are also supported but cannot be shared.

## Transform blocks

Shapes and the camera accept a `<transform name="toWorld">` child that contains one or more transform operations applied left-to-right:

| Element | Attributes | Description |
|---------|------------|-------------|
| `<scale>` | `x`, `y`, `z` | Non-uniform scale. |
| `<translate>` | `x`, `y`, `z` | World-space translation. |
| `<rotate>` | `x`, `y`, `z`, `angle` | Axis-angle rotation in degrees. |
| `<lookAt>` | `origin`, `target`, `up` | Camera-style orientation (sensor only). |

**Note:** operations are applied in the order they appear in the file. To scale then translate, write `<scale>` before `<translate>`.

```xml
<shape type="rectangle">
    <transform name="toWorld">
        <scale x="5" y="1" z="5"/>
        <translate x="0" y="-1" z="0"/>
    </transform>
    <ref id="floor_mat"/>
</shape>
```

`<lookAt>` accepts comma-separated coordinates: `origin="x, y, z"`.
