#!/usr/bin/env python

# 1 happy with 1 dome light with an HDRI
# Copyright (c) 2011-2014 Hiroshi Tsubokawa

import fujiyama

si = fujiyama.SceneInterface()

#plugins
si.OpenPlugin('ConstantShader')
si.OpenPlugin('PlasticShader')

#Camera
si.NewCamera('cam1', 'PerspectiveCamera')
si.SetSampleProperty3('cam1', 'translate', 0, 1, 8.5, 0)

rot = -45

#Light
si.NewLight('light1', 'DomeLight')
si.SetProperty3('light1', 'rotate', 0, rot, 0)
si.SetProperty1('light1', 'sample_count', 256)

#Texture
si.NewTexture('tex1', '../../mip/forrest_salzburg02.mip')
si.AssignTexture('light1', 'environment_map', 'tex1');

#Shader
si.NewShader('happy_shader', 'PlasticShader')
si.SetProperty3('happy_shader', 'diffuse', .8, .8, .8)

si.NewShader('dome_shader', 'ConstantShader')
si.AssignTexture('dome_shader', 'texture', 'tex1')

si.NewShader('sphere_shader1', 'PlasticShader')
si.SetProperty3('sphere_shader1', 'diffuse', 0, 0, 0)
si.SetProperty1('sphere_shader1', 'ior', 40)

si.NewShader('sphere_shader2', 'PlasticShader')
si.SetProperty3('sphere_shader2', 'diffuse', .5, .5, .5)
si.SetProperty3('sphere_shader2', 'reflect', 0, 0, 0)

#Mesh
si.NewMesh('happy_mesh', '../../mesh/happy.mesh')
si.NewMesh('dome_mesh', '../../mesh/dome.mesh')
si.NewMesh('sphere_mesh', '../../mesh/sphere.mesh')

#ObjectInstance
si.NewObjectInstance('happy1', 'happy_mesh')
si.AssignShader('happy1', 'happy_shader')

si.NewObjectInstance('dome1', 'dome_mesh')
si.SetProperty3('dome1', 'rotate', 0, rot, 0)
si.SetProperty3('dome1', 'scale', -.5, .5, .5)
si.AssignShader('dome1', 'dome_shader')

si.NewObjectInstance('sphere1', 'sphere_mesh')
si.AssignShader('sphere1', 'sphere_shader1')
si.SetProperty3('sphere1', 'translate', -1.5, -.5, 0)
si.SetProperty3('sphere1', 'scale', .5, .5, .5)

si.NewObjectInstance('sphere2', 'sphere_mesh')
si.AssignShader('sphere2', 'sphere_shader2')
si.SetProperty3('sphere2', 'translate', 1.5, -.5, 0)
si.SetProperty3('sphere2', 'scale', .5, .5, .5)

#ObjectGroup
si.NewObjectGroup('group1')
si.AddObjectToGroup('group1', 'happy1')
si.AssignObjectGroup('happy1', 'shadow_target', 'group1')

si.NewObjectGroup('group2')
si.AssignObjectGroup('sphere1', 'shadow_target', 'group2')
si.AssignObjectGroup('sphere2', 'shadow_target', 'group2')

si.NewObjectGroup('group3')
si.AddObjectToGroup('group3', 'dome1')
si.AssignObjectGroup('sphere1', 'reflect_target', 'group3')

si.NewObjectGroup('group4')
si.AddObjectToGroup('group4', 'dome1')
si.AddObjectToGroup('group4', 'happy1')
si.AssignObjectGroup('happy1', 'reflect_target', 'group4')

#FrameBuffer
si.NewFrameBuffer('fb1', 'rgba')

#Renderer
si.NewRenderer('ren1')
si.AssignCamera('ren1', 'cam1')
si.AssignFrameBuffer('ren1', 'fb1')
si.SetProperty2('ren1', 'resolution', 640, 480)
#si.SetProperty2('ren1', 'resolution', 160, 120)
#si.SetProperty2('ren1', 'pixelsamples', 9, 9)

#Rendering
si.RenderScene('ren1')

#Output
si.SaveFrameBuffer('fb1', '../dome_light2.fb')

#Run commands
si.Run()
#si.Print()

