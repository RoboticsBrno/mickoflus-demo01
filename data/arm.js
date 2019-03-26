'use strict';

function clampAng(val) {
    val = val % (Math.PI*2);
    if(val < -Math.PI)
        val += Math.PI*2;
    else if(val > Math.PI)
        val -= Math.PI*2;
    return val;
}

function Bone(length, color) {
    this.relAngle = -Math.PI/2;
    this.length = length;
    this.color = color;

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

function Arm(canvasId) {
    this.BODY_HEIGHT = 6;
    this.BODY_RADIUS = 12;
    this.TOUCH_TARGET_SIZE = 4;
    this.ARM_SEGMENTS = [ 11, 10, 8 ];
    this.ARM_COLORS = [ "blue", "orange", "green" ];
    this.ARM_TOTAL_LEN = 0;

    this.bones = [];
    for(var i = 0; i < this.ARM_SEGMENTS.length; ++i) {
        this.ARM_TOTAL_LEN += this.ARM_SEGMENTS[i];
        this.bones.push(new Bone(this.ARM_SEGMENTS[i], this.ARM_COLORS[i]));
    }

    this.bones[2].relMin = -Math.PI/8;
    this.bones[2].relMax = Math.PI/8;

    this.bones[1].relMin = 0.523599;
    this.bones[1].relMax = Math.PI - 0.523599;

    this.bones[0].relMin = -Math.PI;
    this.bones[0].relMax = 0;

    this.canvas = ge1doot.canvas(canvasId);
    this.canvas.resize = this.resize.bind(this);

    this.unit = 1;
    this.origin = {x:0, y:0}
    this.pointer = this.canvas.pointer;

    this.pointer.down = function() {
        requestAnimationFrame(this.run.bind(this));
    }.bind(this);
    this.pointer.up = function() {
        requestAnimationFrame(this.run.bind(this));
    }.bind(this);
    this.pointer.move = function() {
        requestAnimationFrame(this.run.bind(this));
    }.bind(this);

    this.resize();
    this.pointer.x = this.origin.x;
    this.pointer.y = this.origin.y;
    this.run();

    this.touched = false;
}

Arm.prototype.resize = function() {
    this.unit = Math.min(this.canvas.width/2, this.canvas.height *0.7) / (this.ARM_TOTAL_LEN*2.4)

    this.origin.x = this.canvas.width / 2;
    this.origin.y = this.canvas.height * 0.8;

    this.run();
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

Arm.prototype.run = function() {
    var ctx = this.canvas.ctx;
    ctx.clearRect(0, 0, this.canvas.width, this.canvas.height);

    var dx = this.pointer.x - this.origin.x;
    var dy = this.pointer.y - this.origin.y;
    var origAngles = [];
    for(var i = 0; i < this.bones.length; ++i) {
        origAngles.push(this.bones[i].relAngle);
    }
    for(var i = 0; i < 10; ++i) {
        var res = this.solve(dx, dy);
        if(res == 0) {
            continue;
        } else if(res == -1) {
            for(var i = 0; i < this.bones.length; ++i) {
                this.bones[i].relAngle = origAngles[i];
            }
            console.log("FAILED");
        }
        break;
    }

    ctx.save();
    ctx.translate(this.origin.x, this.origin.y);
    var w = this.unit*this.BODY_RADIUS*2;
    var h = this.unit*this.BODY_HEIGHT;

    ctx.fillStyle = "#C8A165";
    ctx.fillRect(-w/2, 6, w, h);

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

/*    for(var i = 0; i < this.bones.length; ++i) {
        var s = this.bones[i];
        this.drawCircleDashed(s.x, s.y, s.length*this.unit, s.color);
    }*/

    ctx.restore();

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
        rotAng = b.rotate(i == 0 ? null : this.bones[i-1], rotAng)
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

