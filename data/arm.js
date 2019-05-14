'use strict';

function clampAng(val) {
    val = val % (Math.PI*2);
    if(val < -Math.PI)
        val += Math.PI*2;
    else if(val > Math.PI)
        val -= Math.PI*2;
    return val;
}

function deg(rad) {
    return rad * (180.0/Math.PI);
}

function Bone(length, color) {
    this.relAngle = -Math.PI/2;
    this.length = length;
    this.color = color;
    this.mapToServoAng = null;

    this.x = 0;
    this.y = 0;
    this.angle = 0;

    this.relMin = -Math.PI;
    this.relMax = -Math.PI;
}

Bone.prototype.updatePos = function(prevBone, unit) {
    this.angle = this.relAngle;
    if(prevBone) {
        this.angle = clampAng(prevBone.angle + this.angle);
    }

    this.x = (Math.cos(this.angle) * this.length * unit);
    this.y = (Math.sin(this.angle) * this.length * unit);

    if(prevBone) {
        this.x += prevBone.x;
        this.y += prevBone.y;
    }
}

function Animation(arm) {
    this.arm = arm;
    this.keyframes = [];
    this.curFrame = -1;
    this.lastTick = null;
}

Animation.prototype.addFrame = function(angles, durationMs) {
    if(angles.length !== this.arm.bones.length) {
        alert("ERROR: Invalid animation angles length.");
        return;
    }

    this.keyframes.push({
        angles: angles,
        duration: durationMs,
        current: 0,
    });
}

Animation.prototype.start = function() {
    this.lastTick = performance.now();
    this.nextFrame();
    requestAnimationFrame(this.update.bind(this));
}

Animation.prototype.nextFrame = function() {
    this.curFrame += 1;
    if(this.curFrame >= this.keyframes.length)
        return false;
    var f = this.keyframes[this.curFrame];
    f.start = [];
    for(var i = 0; i < this.arm.bones.length; ++i) {
        f.start.push(this.arm.bones[i].relAngle);
    }
    return true;
};

Animation.prototype.update = function() {
    var now = performance.now();
    var diff = now - this.lastTick;
    this.lastTick = now;

    var f = this.keyframes[this.curFrame];
    f.current += diff;

    var progress = f.current / f.duration;
    if(progress >= 1.0)
        progress = 1.0;

    for(var i = 0; i < f.angles.length; ++i) {
        diff = f.angles[i] - f.start[i];
        console.log(f.start[i], f.angles[i], diff, clampAng(diff));
        diff = clampAng(diff);
        this.arm.bones[i].relAngle = clampAng(f.start[i] + diff*progress);
    }

    this.arm.updateAngles(true);
    this.arm.draw();

    if(progress >= 1.0 && !this.nextFrame()) {
        this.arm.animation = null;
        return;
    }

    requestAnimationFrame(this.update.bind(this));
}

function Arm(canvasId, manager) {
    this.BODY_HEIGHT = 6;
    this.BODY_RADIUS = 11;
    this.ARM_BASE_HEIGHT = 2;
    this.TOUCH_TARGET_SIZE = 4;
    this.ARM_SEGMENTS = [ 11, 14 ];
    this.ARM_COLORS = [ "blue", "orange" ];
    this.ARM_TOTAL_LEN = 0;
    this.BUTTON_TEXTS = [ "RETRACT", "EXTEND", "GRAB" ];

    this.bones = [];
    this.angles = [];
    for(var i = 0; i < this.ARM_SEGMENTS.length; ++i) {
        this.ARM_TOTAL_LEN += this.ARM_SEGMENTS[i];
        this.bones.push(new Bone(this.ARM_SEGMENTS[i], this.ARM_COLORS[i]));
        this.angles.push(0);
    }

    this.bones[0].relMin = -1.7;
    this.bones[0].relMax = 0;

    this.bones[1].relMin = 0.523599;
    this.bones[1].relMax = Math.PI - 0.261799;

    this.bones[0].mapToServoAng = function() {
        return (Math.PI) - (this.angle * -1) + 0.523599;
    }

    this.bones[1].mapToServoAng = function() {
        var absAng = clampAng(this.angle + Math.PI);
        return Math.PI - (clampAng(absAng + Math.PI/2) * -1) + 0.423599;
    }

    this.buttons = [];
    for(var i = 0; i < this.BUTTON_TEXTS.length; ++i) {
        this.buttons.push({
            "text": this.BUTTON_TEXTS[i],
            "x": 0, "y": 0,
            "w": 0, "h": 0,
            "blink": false,
        });
    }

    this.manager = manager;
    this.canvas = ge1doot.canvas(canvasId);
    this.canvas.resize = this.resize.bind(this);

    this.unit = 1;
    this.origin = {x:0, y:0}
    this.pointer = this.canvas.pointer;

    this.touched = false;
    this.touchedButton = null;
    this.animation = null;

    this.pointer.down = function() {
        this.touchedButton = this.getTouchedButton();
        if(this.touchedButton === null && this.animation === null) {
            this.run();
            this.touched = true;
        }
    }.bind(this);
    this.pointer.up = function() {
        if(this.touchedButton !== null) {
            if(this.getTouchedButton() === this.touchedButton) {
                this.touchedButton.blink = true;
                this.draw();
                setTimeout(function(btn){
                    btn.blink = false;
                    this.draw();
                }.bind(this, this.touchedButton), 100);
                this.handleButton(this.touchedButton.text.toUpperCase());
            }
            this.touchedButton = null;
        } else {
            this.touched = false;
            requestAnimationFrame(this.run.bind(this));
        }
    }.bind(this);
    this.pointer.move = function() {
        if(this.touched)
            requestAnimationFrame(this.run.bind(this));
    }.bind(this);

    this.resize();
    this.pointer.x = this.origin.x;
    this.pointer.y = this.origin.y;
    this.run();

    this.touched = false;
}

Arm.prototype.shouldSend = function() {
    return this.touched || this.animation !== null;
}

Arm.prototype.resize = function() {
    this.unit = Math.min(this.canvas.width*0.6, this.canvas.height *0.8) / this.ARM_TOTAL_LEN;

    var w = this.canvas.width / this.buttons.length;
    var h = this.canvas.height*0.15;
    var y = this.canvas.height - h;
    var x = 0;
    for(var i = 0; i < this.buttons.length; ++i) {
        var b = this.buttons[i];
        b.x = x;
        b.y = y;
        b.w = w;
        b.h = h;
        x += w;
    }

    this.origin.x = this.BODY_RADIUS*this.unit;
    this.origin.y = y - this.BODY_HEIGHT*1.3*this.unit - this.unit*this.ARM_BASE_HEIGHT;

    this.run();
}

Arm.prototype.getTouchedButton = function() {
    var x = this.pointer.x;
    var y = this.pointer.y;
    for(var i = 0; i < this.buttons.length; ++i) {
        var b = this.buttons[i];
        if(x > b.x && x < b.x+b.w && y > b.y && y < b.y+b.h)
            return b;
    }
    return null;
}

Arm.prototype.handleButton = function(text) {
    switch(text) {
        case "RETRACT":
            if(this.animation !== null)
                 break;
            this.animation = new Animation(this);
            this.animation.addFrame([ -1.06, 1.98 ], 300);
            this.animation.addFrame([ -1.70, 2.87 ], 200);
            this.animation.start();
            break;
        case "EXTEND":
            if(this.animation !== null)
                 break;
            this.animation = new Animation(this);
            this.animation.addFrame([ -1.06, 1.98 ], 300);
            this.animation.addFrame([ -0.52, 1.78 ], 200);
            this.animation.start();
            break;
            break;
        case "GRAB":
            this.manager.sendMustArrive("grab", {});
            break;
    }
}

Arm.prototype.drawSegment = function(seg, color) {
    this.drawLine(seg.sx, seg.sy, seg.ex, seg.ey, color, 3, 6);
}

Arm.prototype.drawPointer = function(src, dst, color) {
    var ctx = this.canvas.ctx;
    ctx.beginPath();
    ctx.strokeStyle = color;
    ctx.fillStyle = color;
    ctx.lineWidth = 2;
    ctx.setLineDash([ 6, 3 ]);
    ctx.moveTo(src.x, src.y);
  //  ctx.lineTo(dst.x, dst.y);
    ctx.moveTo(dst.x+this.TOUCH_TARGET_SIZE*2, dst.y);
    ctx.arc(dst.x, dst.y, this.TOUCH_TARGET_SIZE*2, 0, 2 * Math.PI);
    ctx.stroke();
    ctx.setLineDash([]);
}

Arm.prototype.drawCircleDashed = function(x, y, radius, color) {
    var ctx = this.canvas.ctx;
    ctx.beginPath();
    ctx.strokeStyle = color;
    ctx.lineWidth = 2;
    ctx.setLineDash([ 6, 3 ]);
    ctx.moveTo(x, y);
    ctx.arc(x, y, radius, 0, 2 * Math.PI);
    ctx.stroke();
    ctx.setLineDash([]);
}

Arm.prototype.drawTouchTarget = function(x, y) {
    var ctx = this.canvas.ctx;
    ctx.beginPath();
    ctx.fillStyle = "red";
    ctx.moveTo(x, y);
    ctx.arc(x, y, this.TOUCH_TARGET_SIZE, 0, 2 * Math.PI);
    ctx.fill();
}

Arm.prototype.drawLine = function(x0, y0, x1, y1, color, width, dotRadius) {
    var ctx = this.canvas.ctx;
    ctx.beginPath();
    ctx.strokeStyle = color;
    ctx.fillStyle = color;
    ctx.lineWidth = width;
    ctx.moveTo(x0, y0);
    ctx.lineTo(x1, y1);
    ctx.stroke();
    if(dotRadius !== undefined) {
        ctx.moveTo(x0, y0);
        ctx.arc(x0, y0, dotRadius, 0, 2 * Math.PI);
        ctx.moveTo(x1, y1);
        ctx.arc(x1, y1, dotRadius, 0, 2 * Math.PI);
        ctx.fill();
    }
}

Arm.prototype.updateAngles = function(updateAbsAngles) {
    var prev = null;
    for(var i = 0; i < this.bones.length; ++i) {
        if(updateAbsAngles === true)
            this.bones[i].updatePos(prev, this.unit);
        this.angles[i] = deg(this.bones[i].mapToServoAng());
        prev = this.bones[i];
    }
}

Arm.prototype.run = function() {
    var dx = this.pointer.x - this.origin.x;
    var dy = this.pointer.y - this.origin.y;
    for(var i = 0; i < 10; ++i) {
        var res = this.solve(dx, dy);
        if(res == 0) {
            continue;
        } else if(res == -1) {
            //console.log("FAILED");
        }
        break;
    }

    this.updateAngles();
    this.draw();
}

Arm.prototype.draw = function() {
    var ctx = this.canvas.ctx;
    ctx.clearRect(0, 0, this.canvas.width, this.canvas.height);

    ctx.font = '12px monospace';
    ctx.fillStyle = "black"
    var dx = (this.pointer.x - this.origin.x) / this.unit * 10;
    var dy = (this.pointer.y - this.origin.y) / this.unit * 10;
    ctx.fillText(dx.toFixed(1), 20, 12);
    ctx.fillText(dy.toFixed(1), 20, 24);

    ctx.save();
    ctx.translate(this.origin.x, this.origin.y);
    var w = this.unit*this.BODY_RADIUS*2;
    var h = this.unit*this.BODY_HEIGHT;

    ctx.fillStyle = "#C8A165";
    ctx.fillRect(-w/2, this.ARM_BASE_HEIGHT*this.unit, w, h);

    for(var i = 0; i < this.bones.length; ++i) {
        var s = this.bones[i];

        ctx.save();
        ctx.rotate(s.relAngle);
        this.drawLine(0, 0, s.length*this.unit, 0, s.color, 3, 6);

        ctx.translate(s.length*this.unit, 0);
    }

    for(var i = 0; i < this.bones.length; ++i) {
        ctx.restore();
    }

    /*
    ctx.font = '18px monospace';
    ctx.fillStyle = "black"
    var y = this.BODY_HEIGHT*this.unit;
    for(var i = 0; i < this.bones.length; ++i) {
        var b = this.bones[i];

        ctx.save()
        ctx.rotate(b.mapToServoAng());
        this.drawLine(0, 0, 5*this.unit, 0, b.color, 2, 0);
        ctx.restore();

        ctx.fillText((b.angle >= 0 ? " " : "") + b.angle.toFixed(2), -100, y);
        ctx.fillText((b.angle >= 0 ? " " : "") + deg(b.angle).toFixed(2), 0, y);
        ctx.fillText((b.relAngle >= 0 ? " " : "") + b.relAngle.toFixed(2), 100, y);
        ctx.fillText((b.relAngle >= 0 ? " " : "") + deg(b.relAngle).toFixed(2), 200, y);
        ctx.fillText((b.mapToServoAng() >= 0 ? " " : "") + deg(b.mapToServoAng()).toFixed(2), 300, y);

        var relBase = this.bones[i].angle - this.bones[0].angle;
        ctx.fillText((relBase >= 0 ? " " : "") + relBase.toFixed(2), -350, y);
        y += 20;
    }*/

    ctx.restore();

    ctx.strokeStyle = 'blue';
    ctx.lineWidth = 3;
    ctx.textAlign = "center";
    ctx.textBaseline = "middle";
    ctx.font = this.buttons[0].h/3 + "px monospace";
    ctx.fillStyle = "black";
    for(var i = 0; i < this.buttons.length; ++i) {
        var b = this.buttons[i];
        if(b.blink) {
            ctx.fillStyle = "red";
            ctx.fillRect(b.x, b.y, b.w, b.h);
            ctx.fillStyle = "black";
        }
        ctx.strokeRect(b.x, b.y, b.w, b.h);
        ctx.fillText(b.text, b.x + b.w/2, b.y + b.h/2);
    }

    this.drawPointer(this.origin, this.pointer, "red");
}

Bone.prototype.rotate = function(prev, rotAng) {
    var newRelAng = clampAng(this.relAngle + rotAng)
    var _min = this.relMin;
    var _max = this.relMax;
    if(newRelAng < _min) {
        newRelAng = _min;
    } else if(newRelAng > _max) {
        newRelAng = _max;
    }

    var res = clampAng(newRelAng - this.relAngle);
    this.relAngle = newRelAng;
    return res;
}

Arm.prototype.solve = function(targetX, targetY) {
    var prev = null;
    for(var i = 0; i < this.bones.length; ++i) {
        this.bones[i].updatePos(prev, this.unit);
        prev = this.bones[i];
    }

    // Limit under-robot positions
    if(targetX < this.unit*this.BODY_RADIUS) {
        if(targetY > this.unit*this.ARM_BASE_HEIGHT)
            targetY = this.unit*this.ARM_BASE_HEIGHT;
    } else {
        if(targetY > this.unit*(this.ARM_BASE_HEIGHT + this.BODY_HEIGHT))
            targetY = this.unit*(this.ARM_BASE_HEIGHT + this.BODY_HEIGHT);
    }

    var endX = prev.x;
    var endY = prev.y;

    var modifiedBones = false;
    for(var i = this.bones.length-1; i >= 0; --i) {
        var b = this.bones[i];

        var bx = 0
        var by = 0
        if(i > 0) {
            bx = this.bones[i-1].x;
            by = this.bones[i-1].y;
        }

        // Get the vector from the current bone to the end effector position.
        var curToEndX = endX - bx;
        var curToEndY = endY - by;
        var curToEndMag = Math.sqrt(curToEndX*curToEndX + curToEndY*curToEndY);

        // Get the vector from the current bone to the target position.
        var curToTargetX = targetX - bx;
        var curToTargetY = targetY - by;
        var curToTargetMag = Math.sqrt(curToTargetX*curToTargetX + curToTargetY*curToTargetY );

         // Get rotation to place the end effector on the line from the current
        // joint position to the target postion.
        var cosRotAng;
        var sinRotAng;
        var endTargetMag = (curToEndMag*curToTargetMag);

        if(endTargetMag <= 0.0001)
        {
            cosRotAng = 1;
            sinRotAng = 0;
        }
        else
        {
            cosRotAng = (curToEndX*curToTargetX + curToEndY*curToTargetY) / endTargetMag;
            sinRotAng = (curToEndX*curToTargetY - curToEndY*curToTargetX) / endTargetMag;
        }

        // Clamp the cosine into range when computing the angle (might be out of range
        // due to floating point error).
        var rotAng = Math.acos(Math.max(-1, Math.min(1, cosRotAng)));
        if(sinRotAng < 0.0)
            rotAng = -rotAng;

        // Rotate the current bone in local space (this value is output to the user)
        // b.angle = overflow(b.angle + rotAng, Math.PI*2);
        rotAng = this.rotateArm(this.bones, i, rotAng)
        cosRotAng = Math.cos(rotAng)
        sinRotAng = Math.sin(rotAng)

        // Rotate the end effector position.
        endX = bx + cosRotAng*curToEndX - sinRotAng*curToEndY;
        endY = by + sinRotAng*curToEndX + cosRotAng*curToEndY;

        // Check for termination
        var endToTargetX = (targetX-endX);
        var endToTargetY = (targetY-endY);
        if( endToTargetX*endToTargetX + endToTargetY*endToTargetY <= 10) {
            // We found a valid solution.
            return 1;
        }

        // Track if the arc length that we moved the end effector was
        // a nontrivial distance.
        if(!modifiedBones && Math.abs(rotAng)*curToEndMag > 0.000001)
            modifiedBones = true;
    }

    if(modifiedBones)
        return 0;
    return -1;
}

Arm.prototype.rotateArm = function(bones, idx, rotAng) {
    var me = bones[idx];

    var base = bones[0];

    var newRelAng = clampAng(me.relAngle + rotAng)
    var _min = me.relMin;
    var _max = me.relMax;
    if(newRelAng < _min) {
        newRelAng = _min;
    } else if(newRelAng > _max) {
        newRelAng = _max;
    }

    var x = 0;
    var y = 0;
    var prevAng = 0;
    for(var i = 0; i < bones.length; ++i) {
        var b = bones[i];
        var angle = b.relAngle;
        if(idx == i) {
            angle = newRelAng;
        }
        angle = clampAng(prevAng + angle);

        // arm helper-sticks collision with the bottom of the servo stand
        if(i > 0 && i == idx && angle < -0.35) {
            newRelAng = clampAng(-0.35 - prevAng);
            angle = -0.35;
        }

        var nx = x + (Math.cos(angle) * b.length * this.unit);
        var ny = y + (Math.sin(angle) * b.length * this.unit);

        // Limit under-robot positions
        if(nx < this.unit*this.BODY_RADIUS) {
            if(ny > this.unit*this.ARM_BASE_HEIGHT)
                return 0;
        } else {
            if(ny > this.unit*(this.ARM_BASE_HEIGHT + this.BODY_HEIGHT))
                return 0;
        }

        if(i > 0 && angle - base.angle < 0.70) { // arm helper-sticks collision - when extending the arm all the way forward
            base.angle = clampAng(angle-0.70)
        } else if(i > 0 && angle - base.angle > 2.80) {  // arm helper-sticks collision - when fully retracted
            base.angle = clampAng(angle-2.80)
        }

        x = nx;
        y = ny;
        prevAng = angle;
    }

    var res = clampAng(newRelAng - me.relAngle);
    me.relAngle = newRelAng;
    return res;
}
