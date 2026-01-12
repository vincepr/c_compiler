@tool
extends "res://addons/graphtastic/dialog_player/normal_mode/choices/gt_vbox_choices.gd"
@export var singleton : NodePath

func set_GTD():
	GTD=get_node(singleton)
