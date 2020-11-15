<template>
  <div>
    <input ref="colorPicker" :input="cppHexToJs(value)" type="color" hidden @input="$emit('input', jsHexToCpp($event.target.value))"/>
    <md-field class="color-select">
      <label> {{ label }} <em v-bind:style="{color: cppHexToJs(value)}">Color</em></label>
      <md-input readonly :value="cppHexToJs(value)" @click="$refs.colorPicker.click()"></md-input>
    </md-field>
  </div>
</template>

<script>
export default {
  name: "ColorPicker",
  props: ['value', 'label']
}
</script>

<style scoped>
.color-select {
}
</style>
