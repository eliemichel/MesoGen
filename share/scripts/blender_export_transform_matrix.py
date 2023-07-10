import bpy
import struct

def write_view_matrix(context, filepath):
    cam = context.scene.camera
    start = context.scene.frame_start
    end = context.scene.frame_end
    print("Exporting view matrix of camera '" + cam.name + "' between frames " + str(start) + " and " + str(end))
    print(" -- packed as 32 bit floats, 4x4 column major matrix (glm compatible) --")
    with open(filepath, 'wb') as f:
        for frame in range(start, end + 1):
            context.scene.frame_set(frame)
            mat = cam.matrix_world.inverted()
            column_major_data = tuple(mat[i][j] for j in range(4) for i in range(4))
            f.write(struct.pack("f"*16, *column_major_data))

    return {'FINISHED'}

def write_model_matrix(context, filepath):
    obj = context.active_object
    start = context.scene.frame_start
    end = context.scene.frame_end
    print("Exporting model matrix of object '" + obj.name + "' between frames " + str(start) + " and " + str(end))
    print(" -- packed as 32 bit floats, 4x4 column major matrix (glm compatible) --")
    with open(filepath, 'wb') as f:
        for frame in range(start, end + 1):
            context.scene.frame_set(frame)
            mat = obj.matrix_world
            column_major_data = tuple(mat[i][j] for j in range(4) for i in range(4))
            f.write(struct.pack("f"*16, *column_major_data))

    return {'FINISHED'}

from bpy_extras.io_utils import ExportHelper
from bpy.props import StringProperty, BoolProperty, EnumProperty
from bpy.types import Operator


class ExportCameraMatrixBuffer(Operator, ExportHelper):
    """Export the camera view matrix as a raw binary buffer"""
    bl_idname = "export.camera_matrix_buffer"  # important since its how bpy.ops.import_test.some_data is constructed
    bl_label = "Export Camera Matrix Buffer"

    filename_ext = ".bin"

    filter_glob: StringProperty(
        default="*.bin",
        options={'HIDDEN'},
        maxlen=255,  # Max internal buffer length, longer would be clamped.
    )

    def execute(self, context):
        return write_view_matrix(context, self.filepath)

class ExportModelMatrixBuffer(Operator, ExportHelper):
    """Export the object's model matrix as a raw binary buffer"""
    bl_idname = "export.model_matrix_buffer"  # important since its how bpy.ops.import_test.some_data is constructed
    bl_label = "Export Model Matrix Buffer"

    filename_ext = ".bin"

    filter_glob: StringProperty(
        default="*.bin",
        options={'HIDDEN'},
        maxlen=255,  # Max internal buffer length, longer would be clamped.
    )

    def execute(self, context):
        return write_model_matrix(context, self.filepath)

# Only needed if you want to add into a dynamic menu
def menu_func_export(self, context):
    self.layout.operator(ExportCameraMatrixBuffer.bl_idname, text="Export Camera Matrix Buffer")
    self.layout.operator(ExportModelMatrixBuffer.bl_idname, text="Export Model Matrix Buffer")


def register():
    bpy.utils.register_class(ExportCameraMatrixBuffer)
    bpy.utils.register_class(ExportModelMatrixBuffer)
    bpy.types.TOPBAR_MT_file_export.append(menu_func_export)


def unregister():
    bpy.utils.unregister_class(ExportCameraMatrixBuffer)
    bpy.utils.unregister_class(ExportModelMatrixBuffer)
    bpy.types.TOPBAR_MT_file_export.remove(menu_func_export)


if __name__ == "__main__":
    register()
