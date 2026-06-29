shader_type canvas_item;

uniform vec2 shine_intensity_range = vec2(0.0, 1.0);
uniform float shine_frequency = 1.0;
uniform bool shine = true;
//WHY DID I DO THIS I WAS SUPPOSED TO DO EVERYTHING ELSE FIST WHAT

varying vec4 custom_data2;

void vertex() {
	vec4 custom_data = INSTANCE_CUSTOM;
	UV = (custom_data.xy + UV) * custom_data.zw;
}

void fragment(){
	if (shine){
		vec4 color = texture(TEXTURE, UV);
		color.rgb *= mix(shine_intensity_range.x, shine_intensity_range.y, abs(sin(TIME * shine_frequency)));
		COLOR = color * COLOR;
	}
}
