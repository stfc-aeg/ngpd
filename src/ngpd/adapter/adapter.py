"""Odin Control Adapter Class Module."""

import logging

from odin_control.adapters.adapter import ApiAdapter
from odin_control.adapters.response import ApiAdapterResponse
from odin_control.adapters.util import require_controller, response_types
from tornado.httputil import HTTPServerRequest

from ngpd.adapter.controller import NgpdController
from ngpd.adapter.exception import NgpdError

from .._version import __version__


class NgpdAdapter(ApiAdapter):
    """Ngpd Adapter class inheriting base adapter functionality."""

    controller_cls = NgpdController
    error_cls = NgpdError
    version = __version__

    @response_types("application/json", "application/vnd.odin-native", "application/x-hdf5", default="application/json")
    @require_controller
    def get(self, path: str, request: HTTPServerRequest):
        """Handle a HTTP Get request, with overwridden handling for non-json requests."""
        stripped_path = path.strip("/")
        if stripped_path.startswith("data"):
            try:
                splitpath = path.split("/", 1)
                rest = splitpath[1] if len(splitpath) > 1 else None
                logging.debug(f"GET request on special data path: {rest}")
                ctrl: NgpdController = self.controller
                response, filetype = ctrl.get_data(rest)
                content_type = "application/x-hdf5" if filetype == "hdf5" else "application/json"
                status_code = 200
            except self.error_cls as err:
                response = {"error": str(err)}
                status_code = 400
                content_type = "application/json"
            return ApiAdapterResponse(response, content_type=content_type, status_code=status_code)
        else:
            return super().get(path, request)
    
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
