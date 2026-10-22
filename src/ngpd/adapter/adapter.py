"""Odin Control Adapter Class Module."""

from odin_control.adapters.adapter import ApiAdapter
from odin_control.adapters.response import ApiAdapterResponse
from odin_control.adapters.util import require_controller
from tornado.httputil import HTTPServerRequest

from ngpd.adapter.controller import NgpdController
from ngpd.adapter.exception import NgpdError

from .._version import __version__


class NgpdAdapter(ApiAdapter):
    """Ngpd Adapter class inheriting base adapter functionality."""

    controller_cls = NgpdController
    error_cls = NgpdError
    version = __version__

    @require_controller
    def post(self, path: str, request: HTTPServerRequest):
        """Handle a HTTP Post request.

        This overwridden method includes custom handling for multipart/form-data MIME types.
        """
        accept_type = (
            request.headers["Content-Type"].split(";")[0]
            if "Content-Type" in request.headers
            else "*/*"
        )
        if (
            accept_type == "multipart/form-data"
            and path.strip("/") == "config/settings_file/load"
        ):
            try:
                file = request.files["file"][0]
                ctrl: NgpdController = self.controller

                saved_filename = ctrl.device.upload_config(file.filename, file.body)
                ctrl.device.load_config(saved_filename)
                response = {"Config File Uploaded": saved_filename}
                status_code = 201
            except self.error_cls as error:
                response = {"error": str(error)}
                status_code = 400
            return ApiAdapterResponse(
                response, content_type="application/json", status_code=status_code
            )
        else:
            return super().post(path, request)
