<template>
  <div class="widget" ref="widget" v-bind:style="widgetTransform">
    <div class="widget-content" v-bind:style="widgetStyle">
      <ImagePreview v-if="widget.type === 0 || widget.type === 1" :pixel-size="pixelSize" :interval="widget.frequency"
                    :image="widget.content" :frame="widget.offset || 0" :frames="widget.length"
                    :canvas-height="pixelSize * widget.height" :canvas-width="pixelSize * widget.width"></ImagePreview>
      <div v-else-if="widget.type === 4"
           v-bind:style="{color: cppHexToJs(widget.colors[0]), fontSize: `${getWidgetMinimumSize(widget).height * pixelSize}px`}"> {{ widget.content }}
      </div>
      <div v-else-if="widget.type === 5 || widget.type === 6" v-bind:style="{fontSize: `${widget.width < 15 ? 20 : createDefaultWidget(widget.type).height * pixelSize}px`}">
        API
      </div>
      <div v-else-if="widget.type === 8" v-bind:style="shapeStyle"></div>
      <img v-else class="image-pixelated" :src="widgetIcon" alt="Widget Icon" :style="iconStyle">
    </div>
  </div>
</template>

<script>
import ImagePreview from "@/components/ImagePreview";

export default {
  name: "Widget",
  components: {ImagePreview},
  data: () => ({
    rawX: 0,
    rawY: 0,
    rawWidth: 1,
    rawHeight: 1,
    selected: 0,
    nonResizeableTypes: [0, 1],
    currentWidget: {},
    interactable: null,
    resizing: false,
    minimumWidth: 1,
    minimumHeight: 1
  }),
  computed: {
    widgetTransform: function() {
      return {
        width: `${this.widget.width * this.pixelSize}px`,
        height: `${this.widget.height * this.pixelSize}px`,
        'z-index': this.widget.id + 100,
        transform: `translate(${this.widget.xOff * this.pixelSize}px, ${this.widget.yOff * this.pixelSize}px)`,
        outlineWidth: '1px',
        outlineColor: 'black',
        outlineStyle: 'solid'
      }
    },
    widgetStyle: function () {
      return {
        width: `${this.widget.width * this.pixelSize}px`,
        height: `${this.widget.height * this.pixelSize}px`,
        backgroundColor: this.widget.transparent || this.widget.type === 3 ? null : this.cppHexToJs(this.widget.backgroundColor),
        outlineWidth: `${this.widget.type === 3 ? 0 : (this.widget.bordered ? this.pixelSize : 0)}px`,
        outlineColor: this.widget.bordered ? this.cppHexToJs(this.widget.borderColor) : '#000000',
        outlineStyle: 'solid',
        outlineOffset: `${this.widget.bordered ? -this.pixelSize : -1}px`,
        borderRadius: this.widget.type === 8 ? (this.widget.contentType === 1 ? `${this.pixelSize * 2}px` : (this.widget.contentType === 2 ? '50%' : '')) : ''
      }
    },
    iconStyle: function() {
      if (this.widget.type === 3)
        return {width: '100%', height: '100%'}
      let widget = this.getWidgetMinimumSize(this.widget)
      return {
        width: `${(widget.width - (this.widget.bordered ? 2 : 0)) * this.pixelSize}px`,
        height: `${(widget.height - (this.widget.bordered ? 2 : 0)) * this.pixelSize}px`
      }
    },
    shapeStyle: function() {
      if (this.widget.contentType === 3)
        return {}
      return {
        width: `${this.widget.width * this.pixelSize}px`,
        height: `${this.widget.height * this.pixelSize}px`,
        border: `${this.pixelSize}px solid ${this.cppHexToJs(this.widget.colors[0])}`,
        borderRadius: this.widget.contentType === 1 ? `${this.pixelSize * 2}px` : (this.widget.contentType === 2 ? '50%' : '')
      }
    },
    widgetIcon: function () {
      switch (this.widget.type) {
        case 3:
          return `analogClock${this.widget.bordered ? '-bordered' : ''}.png`
        case 7:
          return 'weatherIcon.png'
        case 2:
          return `${this.widget.font >= 2 ? 'bigClock' : 'clock'}${this.widget.contentType}.png`
      }
    }
  },
  watch: {
    widget: {
      deep: true,
      handler: function (widget) {
        if (!this.resizing && !this.areObjectsEqual(widget, this.currentWidget, ['xOff', 'yOff']) ) {
          this.interactable.unset()
          this.setup(widget)
        }
      }
    }
  },
  methods: {
    constrainRawValue(value, min, max) {
      return Math.min(max, Math.max(min, Math.round(value / this.pixelSize)))
    },
    updateCoords() {
      this.widget.xOff = this.constrainRawValue(this.rawX, 0, this.$store.state.stats.width - 1)
      this.widget.yOff = this.constrainRawValue(this.rawY, 0, this.$store.state.stats.height - 1)
      if (this.widget.type !== 3 && this.widget.type !== 8 || (this.widget.type === 8 && this.widget.contentType !== 2)) {
        this.widget.width = this.constrainRawValue(this.rawWidth, 1, this.$store.state.stats.width * 2)
        this.widget.height = this.constrainRawValue(this.rawHeight, 1, this.$store.state.stats.height * 2)
      } else {
        let length = Math.min(this.$store.state.stats.width * 2, this.$store.state.stats.height * 2)
        this.widget.width = this.constrainRawValue(this.rawWidth, 1, length)
        this.widget.height = this.constrainRawValue(this.rawHeight, 1, length)
        this.widget.height = this.widget.width = Math.max(this.widget.height, this.widget.width)
      }
    },
    resetRawCoords(widget) {
      this.rawX = widget.xOff * this.pixelSize
      this.rawY = widget.yOff * this.pixelSize
      this.rawWidth = this.pixelSize * widget.width
      this.rawHeight = this.pixelSize * widget.height
    },
    setup(widget) {
      let minimumWidget = this.getWidgetMinimumSize(widget)
      this.minimumWidth = minimumWidget.width
      this.minimumHeight = minimumWidget.height
      widget.xOff = Math.min(widget.xOff, this.$store.state.stats.width - 1)
      widget.yOff = Math.min(widget.yOff, this.$store.state.stats.height - 1)
      widget.width = Math.min(Math.max(widget.width, this.minimumWidth), this.$store.state.stats.width * 2)
      widget.height = Math.min(Math.max(widget.height, this.minimumHeight), this.$store.state.stats.height * 2)
      this.currentWidget = this.cloneObject(widget)
      this.resetRawCoords(widget)

      this.interactable = this.$interact(this.$refs.widget)
      this.interactable.draggable({
        onmove: event => {
          this.rawX += event.dx
          this.rawY += event.dy
          this.updateCoords()
        },
        onend: () => {
          this.resetRawCoords(this.widget)
        }
      })

      if (!this.nonResizeableTypes.includes(widget.type) && (widget.type !== 8 || widget.contentType !== 3))
        this.interactable.resizable({
          margin: 5,
          modifiers: [
            this.$interact.modifiers.restrictSize({
              min: {width: this.minimumWidth * this.pixelSize, height: this.minimumHeight * this.pixelSize},
              max: {width: this.$store.state.stats.width * 2 * this.pixelSize, height: this.$store.state.stats.height * 2 * this.pixelSize},
            })
          ],
          edges: {
            top: true,
            left: true,
            bottom: true,
            right: true
          },
          onstart: event => {
            this.resizing = true
          },
          onmove: event => {
            this.rawX += event.deltaRect.left
            this.rawY += event.deltaRect.top
            this.rawWidth = event.rect.width
            this.rawHeight = event.rect.height
            this.updateCoords()
          },
          onend: event => {
            this.resetRawCoords(this.widget)
            this.resizing = false
            this.currentWidget = this.cloneObject(this.widget)
          }
        })
      this.interactable.on('doubletap', event => {
        this.$emit('doubletap')
      })
    }
  },
  props: [
    'pixelSize',
    'widget'
  ],
  mounted() {
    this.setup(this.widget)
  },
  beforeDestroy() {
    this.interactable.unset()
  }
}
</script>

<style scoped>
.widget {
  -ms-touch-action: none;
  touch-action: none;
  user-select: none;
  position: absolute;
}

.widget-content {
  display: flex;
  align-items: center;
  justify-content: center;
}

.text {
  display: flex;
  align-items: center;
  justify-content: center;
}
</style>
