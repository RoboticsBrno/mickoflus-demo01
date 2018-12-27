"use strict";
var ge1doot = ge1doot || {};
ge1doot.canvas = function(id) {
	"use strict";
	var canvas = {width:0, height:0, left:0, top: 0, ctx:null, elem:null};
	var elem = document.getElementById(id);
	if (elem) {
		canvas.elem = elem;
	} else {
		canvas.elem = document.createElement("canvas");
		document.body.appendChild(canvas.elem);
	}
	canvas.elem.onselectstart = function() { return false; }
	canvas.elem.ondragstart = function() { return false; }
	canvas.ctx = canvas.elem.getContext("2d");
	canvas.setSize = function() {
		var o = this.elem;
		var w = this.elem.offsetWidth * 1;
		var h = this.elem.offsetHeight * 1;
		if (w != this.width || h != this.height) {
			for (this.left = 0, this.top = 0; o != null; o = o.offsetParent) {
				this.left += o.offsetLeft;
				this.top  += o.offsetTop;
			}
			this.width = this.elem.width = w;
			this.height = this.elem.height = h;
			this.resize && this.resize();
		}
	}
	window.addEventListener('resize', canvas.setSize.bind(canvas), false);
	canvas.setSize();
	canvas.pointer = {
		x: 0,
		y: 0,
		dx: 0,
		dy: 0,
		startX: 0,
		startY: 0,
		canvas: canvas,
		touchMode: false,
		isDown: false,
		center: function (s) {
			this.dx *= s;
			this.dy *= s;
			endX = endY = 0;
		},
		sweeping: false,
	}
	var started = false, endX = 0, endY = 0;
	var addEvent = function (elem, e, fn) {
		for (var i = 0, events = e.split(','); i < events.length; i++) {
			elem.addEventListener(events[i], fn.bind(canvas.pointer), false );
		}
	}
	var distance = function (dx, dy) {
		return Math.sqrt(dx * dx + dy * dy);
	}
	addEvent(window, "mousemove,touchmove", function (e) {
		e.preventDefault();
		this.touchMode = e.targetTouches;
		var pointer = this.touchMode ? this.touchMode[0] : e;
		this.x = pointer.clientX - this.canvas.left;
		this.y = pointer.clientY - this.canvas.top;
		if (started) {
			this.sweeping = true;
			this.dx = endX - (this.x - this.startX);
			this.dy = endY - (this.y - this.startY);
		}
		if (this.move) this.move(e);
	});
	addEvent(canvas.elem, "mousedown,touchstart", function (e) {
		e.preventDefault();
		this.touchMode = e.targetTouches;
		var pointer = this.touchMode ? this.touchMode[0] : e;
		this.startX = this.x = pointer.clientX - this.canvas.left;
		this.startY = this.y = pointer.clientY - this.canvas.top;
		started = true;
		this.isDown = true;
		if (this.down) this.down(e);
		setTimeout(function () {
			if (!started && Math.abs(this.startX - this.x) < 11 && Math.abs(this.startY - this.y) < 11) {
				if (this.tap) this.tap(e);
			}
		}.bind(this), 200);
	});
	addEvent(window, "mouseup,touchend,touchcancel", function (e) {
		e.preventDefault();
		if (started) {
			endX = this.dx;
			endY = this.dy;
			started = false;
			this.isDown = false;
			if (this.up) this.up(e);
			this.sweeping = false;
		}
	});
	return canvas;
}