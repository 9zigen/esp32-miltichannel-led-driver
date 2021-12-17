<template>
  <input
    v-model="displayValue"
    class="input-text"
    type="number"
    v-bind:minlength="minlength"
    v-bind:maxlength="maxlength"
    v-bind:placeholder="placeholder"
    @blur="isInputActive = false"
    @focus="isInputActive = true"
  >
</template>

<script>
export default {
  name: 'InputNumber',
  props: {
    value: {
      type: [String, Number],
      default: () => '1'
    },
    placeholder: {
      type: String,
      default: () => ''
    },
    minlength: {
      type: String,
      default: () => '0'
    },
    maxlength: {
      type: String,
      default: () => 'any'
    }
  },
  data () {
    return {
      isValid: false,
      isInputActive: false
    }
  },
  computed: {
    displayValue: {
      get: function () {
        if (this.isInputActive) {
          // Cursor is inside the input field. unformat display value for user
          return this.value.toString()
        } else {
          return this.value.toString()
          // User is not modifying now. Format display value for user interface
          // return this.value.toFixed(2).replace(/(\d)(?=(\d{3})+(?:\.\d+)?$)/g, '$1,')
        }
      },
      set: function (modifiedValue) {
        // Recalculate value after ignoring '$' and ',' in user input
        let newValue = parseFloat(modifiedValue.replace(/[^\d.]/g, ''))
        // Ensure that it is not NaN
        if (isNaN(newValue)) {
          newValue = 0
        }
        // Note: we cannot set this.value as it is a 'prop'. It needs to be passed to parent component
        // $emit the event so that parent component gets it
        this.$emit('input', newValue)
      }
    }
  },
  methods: {
    input (value) {
      this.$emit('input', this.validRange(value))
    },
    validRange (value) {
      var pattern = '\\d{' + this.minlength + ',' + this.maxlength + '}'
      var regex = new RegExp(pattern)
      let match = value.match(regex)
      return match[0]
    }
  }
}
</script>

<style lang='scss' scoped>

</style>
