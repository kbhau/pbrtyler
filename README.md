# PBR Tyler

This command line tool takes .png images and outputs seamless tiling textures while trying to preserve features based on height.

![pbrtyler_example](https://user-images.githubusercontent.com/17331426/227790163-a912ece6-fe81-497c-8241-beb328ba7b9e.png)

## Usage

Input directory:
```
in_image_d.png
in_image_hrm.png
in_image_n.png
```

Command line:
```
.\pbrtyler.exe -i [path to]in_image -o out_image
```
Note: Suffixes like _d.png will be added automatically.

Output directory:
```
out_image_d.png
out_image_hrm.png
out_image_n.png
```

## Supported parameters:
TODO

## Workflow

Required maps:
- diffuse map (rgb)
- hrm map (height, roughness, metalness)
- normal map (xyz)

These are obtain from a Blender render using shader AOV and compositing.
![pbrtyler_blender_compositing](https://user-images.githubusercontent.com/17331426/227790197-caacc1ed-14a4-49fb-8f3d-b7ae7d01d5c9.png)

Output image is 2x smaller than the input so if you want a 2k seamless texture you need to provide a 4k render.

## Final thoughts

This tool will work best with random textures like grass, earth, pebbles etc. Your mileage with more ordered features like bricks may vary.

There is still a visible blend between features. Noise influence is added to break the pattern. Using parameters you can maximize the blend region, you will find that the features inside get denser.

